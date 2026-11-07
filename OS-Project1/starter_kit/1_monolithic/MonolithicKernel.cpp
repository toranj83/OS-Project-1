#include "MonolithicKernel.hpp"

namespace os_sim {
namespace monolithic {

MonolithicKernel::MonolithicKernel(VirtualDisk& disk, IVirtualConsoleDevice& console, Tracer& tracer)
    : disk_(disk), console_(console), tracer_(tracer),
      file_engine_(disk.get_total_blocks(),
                   [&disk](size_t id, std::array<uint8_t, DISK_BLOCK_SIZE>& b) { return disk.read_block(id, b); },
                   [&disk](size_t id, const std::array<uint8_t, DISK_BLOCK_SIZE>& b) { return disk.write_block(id, b); }) {
    setup_syscall_table();
}

int MonolithicKernel::syscall(int syscall_num, const SyscallArgs& args) {
    tracer_.record_domain_switch("UserSpace", "MonolithicKernel", "syscall");
    tracer_.record_call_depth(3);

    int result = ErrorCode::InvalidArgument;
    auto it = syscall_table_.find(syscall_num);
    if (it != syscall_table_.end()) {
        result = it -> second(*this, args);
    }

    tracer_.record_domain_switch("MonolithicKernel", "UserSpace", "syscall_return");
    return result;
}

void MonolithicKernel::setup_syscall_table() {
    syscall_table_[SYS_OPEN] = handle_open;
    syscall_table_[SYS_READ] = handle_read;
    syscall_table_[SYS_WRITE] = handle_write;
    syscall_table_[SYS_CLOSE] = handle_close;
    syscall_table_[SYS_CONSOLE_WRITE] = handle_console_write;
}

int MonolithicKernel::handle_open(MonolithicKernel& kernel, const SyscallArgs& args) {
    return kernel.sys_open(args.path, args.mode);
}

int MonolithicKernel::handle_read(MonolithicKernel& kernel, const SyscallArgs& args) {
    return kernel.sys_read(args.handle, args.buffer, args.count);
}

int MonolithicKernel::handle_write(MonolithicKernel& kernel, const SyscallArgs& args) {
    return kernel.sys_write(args.handle, args.write_buffer, args.count);
}

int MonolithicKernel::handle_close(MonolithicKernel& kernel, const SyscallArgs& args) {
    return kernel.sys_close(args.handle);
}

int MonolithicKernel::handle_console_write(MonolithicKernel& kernel, const SyscallArgs& args) {
    return kernel.sys_console_write(args.text);
}

int MonolithicKernel::sys_open(const std::string& path, OpenMode mode) {
    if(path.empty()) return ErrorCode::InvalidArgument;
    if(mode != OpenMode::Read && mode != OpenMode::Write) return ErrorCode::InvalidArgument;

    if(mode == OpenMode::Read) {
        if (!file_engine_.file_exists(path)) return ErrorCode::NotFound;
    } else {
        if(file_engine_.file_exists(path)) {
            file_engine_.truncate_file(path);
        } else if (!file_engine_.create_file(path)) {
            return ErrorCode::NoSpace;
        }
    }

    int fd = allocate_fd();
    if (fd < 0) return ErrorCode::NoSpace;
    KernelFileDesc& entry = fd_table_[fd];
    entry.path = path;
    entry.mode = mode;
    entry.offset = 0;
    entry.is_open = true;
    return fd;
}

int MonolithicKernel::sys_read(int handle, uint8_t* out_buf, size_t max_bytes) {
    if(!is_valid_fd(handle)) return ErrorCode::InvalidHandle;
    KernelFileDesc& entry = fd_table_[handle];
    if (entry.mode == OpenMode::Write) return ErrorCode::PermissionDenied;
    if(max_bytes == 0) return 0;
    if (out_buf == nullptr) return ErrorCode::InvalidArgument;

    int bytes = file_engine_.read_bytes(entry.path, entry.offset, out_buf, max_bytes);
    if (bytes > 0) {
        entry.offset += bytes;
        tracer_.record_data_copy(bytes, "KernelBuffer", "UserBuffer");
    }
    return bytes;
}

int MonolithicKernel::sys_write(int handle, const uint8_t* in_data, size_t count) {
    if(!is_valid_fd(handle)) return ErrorCode::Invalidhandle;
    KernelFileDesc& entry = fd_table_[handle];
    if (entry.mode == OpenMode::Read) return ErrorCode::PermissionDenied;
    if (count == 0) return 0;
    if (in_data == nullptr) return ErrorCode::InvalidArgument;

    int bytes = file_engine_.write_bytes(entry.path, entry.offset, in_data, count);
    if (bytes > 0) {
        entry.offset += bytes;
        tracer_.record_data_copy(bytes, "UserBuffer", "KernelBuffer");
    }
    return bytes;
}

int MonolithicKernel::sys_close(int handle) {
    // TODO
    // Validate and close the file descriptor.

    // YOUR CODE HERE
    (void)handle;
    return ErrorCode::InvalidHandle;
}

int MonolithicKernel::sys_console_write(const std::string& text) {
    console_.write(text);
    return ErrorCode::Success;
}

int MonolithicKernel::allocate_fd() {
    for (size_t i = 0; i < fd_table_.size(); ++i) {
        if (!fd_table_[i].is_open) {
            return static_cast<int>(i);
        }
    }
    fd_table_.push_back(KernelFileDesc{});
    return static_cast<int>(fd_table_.size() - 1);
}

bool MonolithicKernel::is_valid_fd(int handle) const {
    if (handle < 0 || static_cast<size_t>(handle) >= fd_table_.size()) {
        return false;
    }
    return fd_table_[handle].is_open;
}

} // namespace monolithic
} // namespace os_sim
