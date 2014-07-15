/*
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * Test secnfs_lib.cpp
 */

#include "secnfs_lib.h"

#include "secnfs.pb.h"

#include <string>
using std::string;

#include <gtest/gtest.h>

#include <cryptopp/osrng.h>
using CryptoPP::AutoSeededRandomPool;

using namespace secnfs;

namespace secnfs_test {

TEST(KeyCodingTest, Basic) {
        RSAKeyPair kp, kp_copy;
        string pri_str, pub_str;

        EncodeKey(kp.pri_, &pri_str);
        DecodeKey(&(kp_copy.pri_), pri_str);

        EncodeKey(kp.pub_, &pub_str);
        DecodeKey(&(kp_copy.pub_), pub_str);

        EXPECT_EQ(kp, kp_copy);
}


TEST(RSACrypto, Basic) {
        RSAKeyPair kp;
        string plain = "RSACrypto", cipher, recovered;
        RSAEncrypt(kp.pub_, plain, &cipher);
        RSADecrypt(kp.pri_, cipher, &recovered);
        EXPECT_EQ(plain, recovered);
}


TEST(MessageCoding, Basic) {
        KeyFile kf;
        kf.set_creator("creator1");
        kf.set_iv("aaaaa");
        kf.set_signature("bbbbb");
        uint32_t kf_size = kf.ByteSize();

        for (uint32_t align = 8; align <= 1024; align <<= 1) {
                KeyFile kf_new;
                void *buf;
                uint32_t buf_size, msg_size;

                EXPECT_TRUE(EncodeMessage(kf, &buf, &buf_size, align));
                EXPECT_EQ((buf_size & (align - 1)), 0);

                EXPECT_FALSE(DecodeMessage(&kf_new, buf, kf_size + 3, &msg_size));
                EXPECT_EQ(msg_size, kf_size);

                EXPECT_TRUE(DecodeMessage(&kf_new, buf, buf_size, &msg_size));
                EXPECT_EQ(msg_size, kf_size);

                EXPECT_EQ(kf.DebugString(), kf_new.DebugString());

                free(buf);
        }
}

TEST(BlockMap, Basic) {
        BlockMap bm;
        EXPECT_EQ(2, bm.try_insert(0, 2));
        EXPECT_EQ(3, bm.try_insert(4, 3));
        EXPECT_EQ(0, bm.try_insert(5, 3));
        EXPECT_EQ(1, bm.try_insert(3, 9));
        EXPECT_EQ(0, bm.try_insert(3, 9));
        bm.remove(0, 2);
        EXPECT_EQ(3, bm.try_insert(0, 9));
        EXPECT_EQ(9, bm.try_insert(10, 9));
        EXPECT_EQ(0, bm.try_insert(12, 9));
        bm.remove(10, 9);
        EXPECT_EQ(9, bm.try_insert(12, 9));
        EXPECT_EQ(2, bm.try_insert(10, 9));
        EXPECT_EQ(1, bm.try_insert(8, 1));
}

};
