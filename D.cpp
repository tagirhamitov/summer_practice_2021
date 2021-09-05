#include <cstdint>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace math {

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

uint64_t BinPow(uint64_t x, uint64_t y) {
  if (y == 0) {
    return 1;
  }
  if ((y & 1) == 0) {
    uint64_t half = BinPow(x, y >> 1);
    return half * half;
  } else {
    return x * BinPow(x, y - 1);
  }
}

uint64_t Mul(uint64_t a, uint64_t b, uint64_t mod) {
  return (a * b) % mod;
}

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

  [[nodiscard]] std::vector<uint64_t> GetDigits() const {
    return digits_;
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

Number Rebase(const Number& num, uint64_t new_base) {
  Number new_number(new_base, {0});
  for (size_t i = num.Size(); i > 0; --i) {
    new_number *= num.Base();
    new_number += num.GetDigit(i - 1);
  }
  return new_number;
}

class Fq {
 public:
  explicit Fq(uint64_t p, std::vector<uint64_t> coefficients,
              std::vector<uint64_t> base) :
          p_(p),
          coefficients_(std::move(coefficients)),
          base_(std::move(base)) {
    NormalizeBase();
    Reduce();
  }

  [[nodiscard]] uint64_t operator[](size_t i) const {
    return coefficients_[i];
  }

  [[nodiscard]] std::vector<uint64_t> Base() const {
    return base_;
  }

  [[nodiscard]] size_t GetN() const {
    return coefficients_.size();
  }

  [[nodiscard]] uint64_t GetP() const {
    return p_;
  }

  [[nodiscard]] std::vector<uint64_t> Coefficients() const {
    return coefficients_;
  }

 private:
  void NormalizeBase() {
    if (base_.back() == 1) {
      return;
    }
    uint64_t k = BinPow(/*x=*/base_.back(), /*y=*/p_ - 2, /*mod=*/p_);
    for (uint64_t& item : base_) {
      item = Mul(item, k, /*mod=*/p_);
    }
  }

  void Reduce() {
    int n = (int) base_.size() - 1;
    while (coefficients_.size() > n) {
      uint64_t k = coefficients_.back();
      for (int i = 0; i < n; ++i) {
        coefficients_[coefficients_.size() - i - 2] +=
                p_ - Mul(k, base_[n - i - 1], /*mod=*/p_);
        coefficients_[coefficients_.size() - i - 2] %= p_;
      }
      coefficients_.pop_back();
    }
  }

  uint64_t p_;
  std::vector<uint64_t> coefficients_;
  std::vector<uint64_t> base_;
};

Fq operator*(const Fq& f1, const Fq& f2) {
  std::vector<uint64_t> base = f1.Base();
  std::vector<uint64_t> coefficients(f1.GetN() + f2.GetN() - 1);
  for (size_t i = 0; i < f1.GetN(); ++i) {
    for (size_t j = 0; j < f2.GetN(); ++j) {
      coefficients[i + j] += Mul(f1[i], f2[j], /*mod=*/f1.GetP());
      coefficients[i + j] %= f1.GetP();
    }
  }
  return Fq(/*p=*/f1.GetP(), coefficients, base);
}

Fq BinPow(const Fq& x, uint64_t y) {
  if (y == 0) {
    return Fq(/*p=*/x.GetP(), /*coefficients=*/{1}, /*base=*/x.Base());
  }
  if ((y & 1) == 0) {
    Fq half = BinPow(x, y >> 1);
    return half * half;
  } else {
    return x * BinPow(x, y - 1);
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

char DecodeChar(uint64_t num) {
  if (0 <= num && num <= 9) {
    return '0' + num;
  }
  if (10 <= num && num <= 35) {
    return 'A' + num - 10;
  }
  if (36 <= num && num <= 61) {
    return 'a' + num - 36;
  }
  if (num == 62) {
    return ' ';
  }
  if (num == 63) {
    return '.';
  }
  return '\0';
}

math::Number EncodeString(const std::string& s) {
  std::vector<uint64_t> encoded_values;
  encoded_values.reserve(s.size());
  for (char c : s) {
    encoded_values.push_back(EncodeChar(c));
  }
  return {/*Base=*/64, /*digits=*/std::move(encoded_values)};
}

std::string DecodeString(const math::Number& number) {
  std::string s;
  for (size_t i = 0; i < number.Size(); ++i) {
    s.push_back(DecodeChar(number.GetDigit(i)));
  }
  return s;
}

std::vector<math::Fq> SplitBlocks(const std::vector<uint64_t>& sequence,
                                  size_t n, const std::vector<uint64_t>& base,
                                  uint64_t p) {
  std::vector<math::Fq> blocks;
  blocks.reserve(sequence.size() / n + 1);
  std::vector<uint64_t> cur_block;
  for (uint64_t item : sequence) {
    cur_block.push_back(item);
    if (cur_block.size() == n) {
      blocks.emplace_back(p, /*coefficients=*/cur_block, base);
      cur_block.clear();
    }
  }
  if (!cur_block.empty()) {
    blocks.emplace_back(p, /*coefficients=*/cur_block, base);
  }
  return blocks;
}

}  // namespace encoding

namespace crypto {

std::pair<uint64_t, uint64_t>
Encrypt(uint64_t message, uint64_t p, uint64_t g, uint64_t public_key,
        std::mt19937 gen) {
  // Generate random integer from [1, p - 1].
  uint64_t b = 1 + gen() % (p - 1);
  // ElGamal encryption.
  uint64_t g_b = math::BinPow(g, b, /*mod=*/p);
  uint64_t g_ab = math::BinPow(public_key, b, /*mod=*/p);
  uint64_t encrypted = math::Mul(message, g_ab, /*mod=*/p);
  return {g_b, encrypted};
}

uint64_t Decrypt(std::pair<uint64_t, uint64_t> encrypted_message, uint64_t p,
                 uint64_t private_key) {
  uint64_t g_b = encrypted_message.first;
  uint64_t actual_encrypted_message = encrypted_message.second;
  uint64_t g_ab = math::BinPow(g_b, private_key, /*mod=*/p);
  uint64_t g_ab_inv = math::BinPow(g_ab, p - 2, /*mod=*/p);
  return math::Mul(actual_encrypted_message, g_ab_inv, /*mod=*/p);
}

std::pair<math::Fq, math::Fq>
Encrypt(const math::Fq& message, const math::Fq& g, const math::Fq& public_key,
        std::mt19937 gen) {
  uint64_t group_size = math::BinPow(g.GetP(), g.Base().size() - 1);
  uint64_t b = 1 + gen() % (group_size - 1);
  math::Fq g_b = math::BinPow(g, b);
  math::Fq g_ab = math::BinPow(public_key, b);
  math::Fq encrypted = g_ab * message;
  return {g_b, encrypted};
}

math::Fq Decrypt(const std::pair<math::Fq, math::Fq>& encrypted_message,
                 uint64_t private_key) {
  math::Fq g_b = encrypted_message.first;
  math::Fq actual_encrypted_message = encrypted_message.second;
  math::Fq g_ab = math::BinPow(g_b, private_key);
  uint64_t group_size = math::BinPow(g_b.GetP(), g_b.Base().size() - 1);
  math::Fq g_ab_inv = math::BinPow(g_ab, group_size - 2);
  return actual_encrypted_message * g_ab_inv;
}

}  // namespace crypto

namespace string_utils {

std::vector<uint64_t>
SplitAndCastToUint64(const std::string& line, uint64_t p) {
  // Split line into tokens.
  std::vector<std::string> tokens;
  std::string cur_token;
  for (char c : line) {
    if (c == ' ') {
      tokens.push_back(cur_token);
      cur_token = "";
    } else {
      cur_token += c;
    }
  }
  if (!cur_token.empty()) {
    tokens.push_back(cur_token);
  }
  // Convert each token to uint64_t.
  std::vector<uint64_t> coefficients;
  coefficients.reserve(tokens.size());
  for (const std::string& token : tokens) {
    int64_t coefficient = std::stoll(token);
    if (coefficient < 0) {
      coefficient += p;
    }
    coefficients.push_back(coefficient);
  }
  return coefficients;
}

std::vector<uint64_t> ReadPolynomial(std::istream& input, uint64_t p) {
  std::string coefficients;
  std::getline(input, coefficients);
  return SplitAndCastToUint64(coefficients, p);
}

void PrintFq(std::ostream& output, const math::Fq& fq) {
  for (size_t i = 0; i < fq.GetN(); ++i) {
    output << fq[i] << " ";
  }
  output << "\n";
}

}  // namespace string_utils

int main() {
#ifdef LOCAL
  freopen("input.txt", "r", stdin);
  freopen("output.txt", "w", stdout);
#endif
  // Read input.
  uint64_t p;
  std::cin >> p;
  std::cin.ignore();
  std::vector<uint64_t> f = string_utils::ReadPolynomial(std::cin, p);
  uint64_t private_key;
  std::cin >> private_key;
  std::cin.ignore();
  std::vector<std::pair<math::Fq, math::Fq>> encrypted;
  std::string line;
  while (std::getline(std::cin, line)) {
    std::stringstream ss(line);
    math::Fq g_b(p, /*coefficients=*/string_utils::ReadPolynomial(ss, p),
            /*base=*/f);
    std::getline(std::cin, line);
    ss = std::stringstream(line);
    math::Fq encrypted_message(p, /*coefficients=*/
                               string_utils::ReadPolynomial(ss, p),
            /*base=*/f);
    encrypted.emplace_back(g_b, encrypted_message);
  }

  // Decrypt.
  std::vector<math::Fq> blocks;
  blocks.reserve(encrypted.size());
  for (const auto& item : encrypted) {
    blocks.push_back(crypto::Decrypt(item, private_key));
  }

  // Convert to string.
  std::vector<uint64_t> united;
  for (const math::Fq& block : blocks) {
    for (uint64_t item : block.Coefficients()) {
      united.push_back(item);
    }
  }
  math::Number message(p, united);
  message = math::Rebase(message, 64);
  std::string text = encoding::DecodeString(message);

  // Write output.
  std::cout << text << "\n";
  return 0;
}
