
#include <openssl/dh.h>

#include "serialization.hpp"

#include "avjackif.hpp"
#include "avproto.hpp"

#include "avim_proto/message.pb.h"
#include "avim_proto/user.pb.h"

/*
 * 这个类呢，是用来实现和 JACK 实现的那个 AVROUTER 对接的。也就是 JACK 版本的 AV NETWORK SERVICE PROVIDER
 */

// av地址可以从证书里获取，所以外面无需传递进来
avjackif::avjackif(boost::shared_ptr<boost::asio::ip::tcp::socket> _sock)
	: m_sock(_sock)
{

}

avjackif::~avjackif()
{

}


void avjackif::async_handshake(std::string login_username, std::string login_password, boost::asio::yield_context yield_context)
{
	proto::client_hello client_hello;
	client_hello.set_client("avim");
	client_hello.set_version(0001);

	unsigned char to[100];

	auto dh = DH_new();
	DH_generate_parameters_ex(dh,64,DH_GENERATOR_5,NULL);

	// 把 g 和 pubkey 传过去
	client_hello.set_random_g((const void*)to, BN_bn2bin(dh->g, to));
	client_hello.set_random_pub_key((const void*)to, BN_bn2bin(dh->pub_key, to));

	auto tobesend = av_router::encode(client_hello);

	boost::asio::async_write(*m_sock, boost::asio::buffer(tobesend), yield_context);

	std::uint32_t l;
	boost::asio::async_read(*m_sock, boost::asio::buffer(&l, sizeof(l)), boost::asio::transfer_exactly(4), yield_context);
	auto hostl = htonl(l);
	std::string  buf;

	buf.resize(hostl + 4);
	memcpy(&buf[0], &l, 4);
	hostl = boost::asio::async_read(*m_sock, boost::asio::buffer(&buf[4], hostl),
				boost::asio::transfer_exactly(hostl), yield_context);

	// 解码
	boost::scoped_ptr<proto::server_hello> server_hello((proto::server_hello*)av_router::decode(buf));

	DH_generate_key(dh);
	dh->p = BN_bin2bn((const unsigned char *) server_hello->random_p().data(), server_hello->random_p().length(), dh->p);
	dh->pub_key = BN_bin2bn((const unsigned char *) server_hello->random_pub_key().data(), server_hello->random_pub_key().length(), dh->pub_key);

	m_shared_key.resize(DH_size(dh));
	// 密钥就算出来啦！
	DH_compute_key(&m_shared_key[0], dh->pub_key, dh);

    printf("key = 0x");
    for (int i=0; i<DH_size(dh); ++i) {
        printf("%x%x", (m_shared_key[i] >> 4) & 0xf, m_shared_key[i] & 0xf);
    }
    printf("\n");
	DH_free(dh);

	// 接着私钥加密 随机数
	auto singned = RSA_private_encrypt(_rsa, std::string((const char*)m_shared_key.data(), m_shared_key.size()));

	proto::login login_packet;
	login_packet.set_user_name(login_username);
	login_packet.set_encryped_radom_key(singned);

	boost::asio::async_write(*m_sock, boost::asio::buffer(av_router::encode(login_packet)), yield_context);

	// 读取回应
	boost::asio::async_read(*m_sock, boost::asio::buffer(&l, sizeof(l)), yield_context);

	hostl = htonl(l);
	buf.resize(htonl(l) + 4);
	memcpy(&buf[0], &l, 4);
	boost::asio::async_read(*m_sock, boost::asio::buffer(&buf[4], htonl(l)), yield_context);
	// 解码
	boost::scoped_ptr<proto::login_result> login_result((proto::login_result*)av_router::decode(buf));
}
