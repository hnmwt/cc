#pragma once

#include <string>
#include <vector>

namespace inspection {

/**
 * @brief Base64エンコード・デコードユーティリティ
 */
class Base64 {
public:
    /**
     * @brief バイナリデータをBase64エンコード
     * @param data バイナリデータ
     * @return Base64エンコードされた文字列
     */
    static std::string encode(const std::vector<unsigned char>& data);

    /**
     * @brief Base64文字列をデコード
     * @param encoded Base64エンコードされた文字列
     * @return デコードされたバイナリデータ
     */
    static std::vector<unsigned char> decode(const std::string& encoded);

private:
    static const std::string base64_chars;
    static inline bool is_base64(unsigned char c);
};

} // namespace inspection
