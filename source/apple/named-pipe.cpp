#include "named-pipe.hpp"
#include <errno.h>

os::apple::named_pipe::named_pipe(os::create_only_t, const std::string name)
{
    int ret = 0;
    this->name_req = name + "-req";
    this->name_rep = name + "-rep";

    created = true;
}

os::apple::named_pipe::named_pipe(os::open_only_t, const std::string name)
{
    this->name_req = name + "-req";
    this->name_rep = name + "-rep";

    int ret = 0;
    ret = remove(name_req.c_str());
    ret = mkfifo(name_req.c_str(), S_IRUSR | S_IWUSR);
    ret = remove(name_rep.c_str());
    ret = mkfifo(name_rep.c_str(), S_IRUSR | S_IWUSR);

    connected = true;
}

os::apple::named_pipe::~named_pipe() {
    remove(name_req.c_str());
    remove(name_rep.c_str());
}

void read_cb (int sig) {
    // std::cout << "read cb called!" << std::endl;
}

void write_cb (int sig) {
    // std::cout << "write cb called!" << std::endl;
}

uint32_t os::apple::named_pipe::read(char *buffer, size_t buffer_length, bool is_blocking, SocketType t)
{
    os::error err = os::error::Error;
    int ret = 0;
    int sizeChunks = 8*1024; // 8KB
    int offset = 0;
    int file_descriptor = -1;


    while (ret == 0) {
        if (is_blocking)
            file_descriptor = open(t == REQUEST ? name_req.c_str() : name_rep.c_str(), O_RDONLY);
        else
            file_descriptor = open(t == REQUEST ? name_req.c_str() : name_rep.c_str(), O_RDONLY | O_NONBLOCK);

        if (file_descriptor < 0) {
            std::cout << "Could not open read pipe " << strerror(errno) << std::endl;
            break;
        }

        ret = ::read(file_descriptor, buffer, buffer_length);
        while (ret == sizeChunks) {
            std::cout << "chunk data" << std::endl;
            offset += sizeChunks;
            std::vector<char> new_chunks;
            new_chunks.resize(sizeChunks);
            ret = ::read(file_descriptor, new_chunks.data(), new_chunks.size());
            ::memcpy(&buffer[offset], new_chunks.data(), ret);
        }

        // if (ret == 0) {
        //     std::cout << "Could not read from pipe " << strerror(errno) << std::endl;
        // }
        close(file_descriptor);
    }

    err = os::error::Success;
end:
    return (uint32_t) err;
}

uint32_t os::apple::named_pipe::write(const char *buffer, size_t buffer_length, SocketType t)
{
    os::error err = os::error::Error;
    int ret = 0;

    int file_descriptor = open(t == REQUEST ? name_req.c_str() : name_rep.c_str(), O_WRONLY | O_DSYNC);
    if (file_descriptor < 0) {
        std::cout << "Could not open write pipe " << strerror(errno) << std::endl;
        goto end;
    }

    ret = ::write(file_descriptor, buffer, buffer_length);

    // if (ret == 0) {
    //     std::cout << "Could not write from pipe " << strerror(errno) << std::endl;
    //     abort();
    // }
    // if (ret < buffer_length) {
    //     std::cout << "Could not write all the data " << strerror(errno) << std::endl;
    //     abort();
    // }

    err = os::error::Success;
end:
    close(file_descriptor);
    return (uint32_t) err;
}

bool os::apple::named_pipe::is_created() {
	return created;
}

bool os::apple::named_pipe::is_connected() {
    return connected;
}

void os::apple::named_pipe::set_connected(bool is_connected) {
    connected = is_connected;
}

void os::apple::named_pipe::handle_accept_callback(os::error code, size_t length) {
	if (code == os::error::Connected || code == os::error::Success) {
		set_connected(true);
	} else {
		set_connected(false);
	}
}

os::error os::apple::named_pipe::accept(std::shared_ptr<os::async_op> &op, os::async_op_cb_t cb) {
    if (!is_created()) {
		return os::error::Error;
	}

	std::shared_ptr<os::apple::async_request> ar = std::static_pointer_cast<os::apple::async_request>(op);

    if (!ar) {
		ar = std::make_shared<os::apple::async_request>();
		op = std::static_pointer_cast<os::async_op>(ar);
	}

	ar->set_callback(cb);
	ar->set_system_callback(
		std::bind(&named_pipe::handle_accept_callback, this, std::placeholders::_1, std::placeholders::_2));
    ar->set_sem(NULL);
    connected = true;
    
    os::error ec = os::error::Connected;

	if (ec != os::error::Pending && ec != os::error::Connected) {
		ar->call_callback(ec, 0);
		ar->cancel();
		
	} else {
		ar->set_valid(true);
		if (ec == os::error::Connected) {
			ar->call_callback(ec, 0);
		}
	}
	return ec;
}
