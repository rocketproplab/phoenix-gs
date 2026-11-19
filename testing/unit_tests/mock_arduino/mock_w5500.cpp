#include "w5500.h"
#include <vector>

std::vector<std::vector<uint8_t>> mock_sent_packets;
std::vector<std::vector<uint8_t>> mock_rx_queue;

Wiznet5500::Wiznet5500(int8_t cs) {
}

boolean Wiznet5500::begin(const uint8_t *address) {
    return true;
}

void Wiznet5500::end() {
}

uint16_t Wiznet5500::sendFrame(const uint8_t *data, uint16_t datalen) {
    if (data && datalen > 0) {
        mock_sent_packets.push_back(std::vector<uint8_t>(data, data + datalen));
    }
    return datalen;
}

uint16_t Wiznet5500::readFrame(uint8_t *buffer, uint16_t maxlen) {
    if (mock_rx_queue.empty()) {
        return 0;
    }
    std::vector<uint8_t> frame = mock_rx_queue.front();
    mock_rx_queue.erase(mock_rx_queue.begin());
    uint16_t to_copy = (frame.size() < maxlen) ? (uint16_t)frame.size() : maxlen;
    memcpy(buffer, frame.data(), to_copy);
    return to_copy;
}
