#pragma once

#include <string>
#include <cstring>
#include <crypt.h>   // crypt_r 사용

namespace BCrypt {
    // plain: 입력 비밀번호, hash: 저장된 bcrypt 해시($2y$... 같은 형식)
    inline bool validatePassword(const std::string& plain, const std::string& hash) {
        if (hash.empty()) return false;

        struct crypt_data data;
        memset(&data, 0, sizeof(data));

        // crypt_r은 reentrant(스레드 안전) 버전
        char *res = crypt_r(plain.c_str(), hash.c_str(), &data);
        if (!res) return false;

        // crypt_r은 반환된 문자열이 해시와 같은 형식의 전체 문자열을 반환함
        return std::strcmp(res, hash.c_str()) == 0;
    }
}
