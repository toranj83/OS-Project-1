#include "Layer3_OSApi.hpp"

namespace os_sim {
namespace layered {

Layer3_OSApi::Layer3_OSApi(Layer2_FileSystem& layer2, Tracer& tracer)
    : layer2_(layer2), tracer_(tracer) {}

int Layer3_OSApi::open(const std::string& path, OpenMode mode) {
    ParameterBlock pb;
    pb.op = LayerOp::Open;
    pb.path = path;
    pb.mode = mode;
    tracer_.record_layer_transition(3, 2, "OPEN");
    layer2_.process_request(pb);
    return pb.status;
}

int Layer3_OSApi::read(int handle, std::vector<uint8_t>& out_buffer, size_t max_bytes) {
    out_buffer.resize(max_bytes);

    ParameterBlock pb;
    pb.op = LayerOp::Read;
    pb.handle = handle;
    pb.buffer = out_buffer.data();
    pb.length = max_bytes;
    tracer_.record_layer_transition(3, 2, "READ");
    layer2_.process_request(pb);

    if (pb.status <= 0) {
        out_buffer.clear();
    } else {
        out_buffer.resize(static_cast<size_t>(pb.status));
    }
    return pb.status;
}

int Layer3_OSApi::write(int handle, const std::vector<uint8_t>& data) {
    ParameterBlock pb;
    pb.op = LayerOp::Write;
    pb.handle = handle;
    pb.write_buffer = data.data();
    pb.length = data.size();
    tracer_.record_layer_transition(3, 2, "WRITE");
    layer2_.process_request(pb);
    return pb.status;
}

int Layer3_OSApi::close(int handle) {
    // TODO
    // Send a traced close request to Layer 2 and return its status.

    // YOUR CODE HERE
    (void)handle;
    return ErrorCode::InvalidHandle;
}

void Layer3_OSApi::console_write(const std::string& text) {
    // TODO
    // Send a traced console-output request to Layer 2.

    // YOUR CODE HERE
    (void)text;
}

std::string Layer3_OSApi::get_architecture_name() const {
    return "Strict Layered Approach";
}

} // namespace layered
} // namespace os_sim
