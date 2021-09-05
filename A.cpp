#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace math {

class Number {
 public:
  Number(uint64_t base, std::vector<uint64_t> digits) : base_(base),
                                                        digits_(std::move(
                                                                digits)) {}

  Number& operator+=(uint64_t num) {
    digits_[0] += num;
    Normalize();
    return *this;
  }

  Number& operator*=(uint64_t num) {
    uint64_t remainder = 0;
    for (size_t i = 0; i < digits_.size(); ++i) {
      digits_[i] *= num;
      digits_[i] += remainder;
      remainder = Split(i);
    }
    digits_.push_back(remainder);
    Normalize();
    return *this;
  }

  [[nodiscard]] size_t Size() const {
    return digits_.size();
  }

  [[nodiscard]] uint64_t Base() const {
    return base_;
  }

  [[nodiscard]] uint64_t GetDigit(size_t i) const {
    return digits_[i];
  }

 private:
  void Normalize() {
    for (size_t i = 0; i < digits_.size(); ++i) {
      uint64_t remainder = Split(i);
      if (remainder == 0) {
        continue;
      }
      if (i + 1 == digits_.size()) {
        digits_.push_back(remainder);
      } else {
        digits_[i + 1] += remainder;
      }
    }
    while (digits_.size() > 1 && digits_.back() == 0) {
      digits_.pop_back();
    }
  }

  uint64_t Split(size_t i) {
    uint64_t remainder = digits_[i] / base_;
    digits_[i] %= base_;
    return remainder;
  }

  uint64_t base_;
  std::vector<uint64_t> digits_;
};

Number Rebase(Number num, uint64_t new_base) {
  Number new_number(new_base, {0});
  for (size_t i = num.Size(); i > 0; --i) {
    new_number *= num.Base();
    new_number += num.GetDigit(i - 1);
  }
  return new_number;
}

uint64_t BinPow(uint64_t x, uint64_t y, uint64_t mod) {
  if (y == 0) {
    return 1;
  }
  if ((y & 1) == 0) {
    uint64_t half = BinPow(x, y >> 1, mod);
    return (half * half) % mod;
  } else {
    return (x * BinPow(x, y - 1, mod)) % mod;
  }
}

}  // namespace math

namespace encoding {

uint64_t EncodeChar(char c) {
  if (c >= 48 && c <= 57) {
    return c - 48;
  }
  if (c >= 65 && c <= 90) {
    return c - 55;
  }
  if (c >= 97 && c <= 122) {
    return c - 61;
  }
  if (c == 32) {
    return 62;
  }
  if (c == 46) {
    return 63;
  }
  return 64;
}

math::Number EncodeString(const std::string& s) {
  std::vector<uint64_t> encoded_values;
  encoded_values.reserve(s.size());
  for (char c : s) {
    encoded_values.push_back(EncodeChar(c));
  }
  return {/*Base=*/64, /*digits=*/std::move(encoded_values)};
}

}  // namespace encoding

int main() {
#ifdef LOCAL
  freopen("input.txt", "r", stdin);
  freopen("output.txt", "w", stdout);
#endif
  // Read input.
  uint64_t p, g, public_key;
  std::cin >> p >> g >> public_key;
  std::string text;
  std::cin.ignore();
  getline(std::cin, text);

  // Encode message.
  math::Number msg = encoding::EncodeString(text);
  msg = math::Rebase(msg, p);

  // Encrypt message.
  std::mt19937 gen;
  std::vector<std::pair<uint64_t, uint64_t>> encrypted_message;
  encrypted_message.reserve(msg.Size());
  for (size_t i = 0; i < msg.Size(); ++i) {
    uint64_t b = 1 + gen() % (p - 1);
    uint64_t g_b = math::BinPow(g, b, p);
    uint64_t g_ab = math::BinPow(public_key, b, p);
    uint64_t encrypted = (msg.GetDigit(i) * g_ab) % p;
    encrypted_message.emplace_back(g_b, encrypted);
  }

  // Write output.
  for (auto& item : encrypted_message) {
    std::cout << item.first << " " << item.second << "\n";
  }
  return 0;
}
