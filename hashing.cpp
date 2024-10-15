#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cmath>
#include <bitset>
#include <iomanip>

bool DEBUG = false;

void log(std::string message) {
    if (DEBUG) {
        std::cout << "\033[32m" << "#DEBUG " << "\033[0m" << message << std::endl;
    }
}

std::vector<int> get_primes(int count) {
    std::vector<int> primes;
    if (count >= 1) primes.push_back(2);

    int candidate = 3;

    while (primes.size() < static_cast<size_t>(count)) {
        bool is_prime = true;
        int limit = static_cast<int>(sqrt(candidate));

        for (int prime : primes) {
            if (prime > limit) break;
            if (candidate % prime == 0) {
                is_prime = false;
                break;
            }
        }

        if (is_prime) {
            primes.push_back(candidate);
        }
        candidate += 2;
    }

    return primes;
}

uint32_t get_fractional_bits(int prime, const std::string& root_type) {
    double root;
    double fractional_part;
    uint32_t fractional_bits;

    if (root_type == "cube") {
        root = std::cbrt(prime);
    }
    else if (root_type == "square") {
        root = std::sqrt(prime);
    }
    else {
        std::cerr << "Unknown root_type. Use 'cube' or 'square'." << std::endl;
        return 0;
    }

    fractional_part = root - static_cast<int>(root);
    fractional_bits = static_cast<uint32_t>(fractional_part * (1ULL << 32)) & 0xFFFFFFFF;
    return fractional_bits;
}

std::vector<uint32_t> get_constants(const char cste) {
    std::vector<uint32_t> constants;
    int num;
    std::string root_type;

    if (cste == 'K') {
        num = 64;
        root_type = "cube";
    }
    else if (cste == 'H') {
        num = 8;
        root_type = "square";
    }
    else {
        std::cerr << "Unknown constant type." << std::endl;
        return constants;
    }

    std::vector<int> primes = get_primes(num);
    for (int prime : primes) {
        constants.push_back(get_fractional_bits(prime, root_type));
    }
    return constants;
}

const std::vector<uint32_t> K = get_constants('K');
std::vector<uint32_t> H = get_constants('H');
uint32_t rgt_rotate(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

std::vector<uint8_t> pad_message(std::string message) {
    std::vector<uint8_t> padded_message;
    rsize_t byte_size = message.size();
    uint64_t bit_size = byte_size * 8;

    for (char c : message) padded_message.push_back(static_cast<uint8_t>(c));
    padded_message.push_back(0x80);
    while ((padded_message.size() % 64) != 56) padded_message.push_back(0x00);
    for (int i = 7; i >= 0; i--) {
        padded_message.push_back((bit_size >> (i * 8)) & 0xFF);
    }
    log("Message padded");
    return padded_message;
}

void process(std::vector<uint8_t> chunk) {

    std::vector<uint32_t> W(64);
    uint32_t a0;
    uint32_t a1;

    for (int i = 0; i < 16; i++) {
        W[i] = (chunk[i * 4] << 24) | (chunk[i * 4 + 1] << 16) | (chunk[i * 4 + 2] << 8) | chunk[i * 4 + 3];
    }

    for (int i = 16; i < 64; i++) {
        a0 = rgt_rotate(W[i - 15], 7) ^ rgt_rotate(W[i - 15], 18) ^ (W[i - 15] >> 3);
        a1 = rgt_rotate(W[i - 2], 17) ^ rgt_rotate(W[i - 2], 19) ^ (W[i - 2] >> 10);
        W[i] = W[i - 16] + a0 + W[i - 7] + a1;
    }

    uint32_t a = H[0], b = H[1], c = H[2], d = H[3], e = H[4], f = H[5], g = H[6], h = H[7];
    uint32_t S1, S0, ch, Temp1, Temp2, maj;

    for (int i = 0; i < 64; i++) {
        S1 = rgt_rotate(e, 6) ^ rgt_rotate(e, 11) ^ rgt_rotate(e, 25);
        ch = (e & f) ^ (~e & g);
        Temp1 = h + S1 + ch + K[i] + W[i];
        S0 = rgt_rotate(a, 2) ^ rgt_rotate(a, 13) ^ rgt_rotate(a, 22);
        maj = (a & b) | (a & c) | (b & c);
        Temp2 = maj + S0;

        h = g;
        g = f;
        f = e;
        e = d + Temp1;
        d = c;
        c = b;
        b = a;
        a = Temp1 + Temp2;
    }

    H[0] = H[0] + a;
    H[1] = H[1] + b;
    H[2] = H[2] + c;
    H[3] = H[3] + d;
    H[4] = H[4] + e;
    H[5] = H[5] + f;
    H[6] = H[6] + g;
    H[7] = H[7] + h;
    log("SHA256 ended succesfully");
}


int SHA256(std::string message) {
    std::vector<uint8_t> padded_message = pad_message(message);
    std::vector<std::vector<uint8_t>> chunks;
    log("Constants extracted");

    for (int i = 0; i < padded_message.size(); i += 64) {
        std::vector<uint8_t> chunk(padded_message.begin() + i, padded_message.begin() + 1 + 64);
        chunks.push_back(chunk);
    }
    log("Succesfully converted to chunks");

    for (std::vector<uint8_t> chunk : chunks) {
        process(chunk);
    }
}

int main(int argc, char* argv[]) {
    std::string message;

    if (argc > 1) {
        message = argv[1];

        if (argc > 2 && argv[2][0] == '1') {
            DEBUG = true;
        }
    }

    SHA256(message);
    std::cout << "Final hash is: ";
    for (uint32_t p : H) {
        std::cout << std::hex << std::setw(8) << std::setfill('0') << p ;
    }
    std::cout << std::endl;


    return 0;
}
