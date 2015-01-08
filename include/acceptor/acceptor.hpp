

#pragma once

#include <utility>
#include <type_traits>
#include <boost/system/error_code.hpp>

namespace acceptor {

	namespace detail {

		template<class AcceptSocket, class ClientSocket, class SocketCreator, class AcceptHandler, class CompleteHandler>
		struct async_accept_op
		{
			AcceptSocket& m_socket;

			SocketCreator m_socket_creator;
			AcceptHandler m_accept_handler;
			CompleteHandler m_complete_handler;

			std::shared_ptr<ClientSocket> m_accepted_socket_store;

			async_accept_op(AcceptSocket& sock, SocketCreator&& sc, AcceptHandler&& accepthandler, CompleteHandler&& complethandler)
				: m_socket(sock)
				, m_socket_creator(sc)
				, m_accept_handler(accepthandler)
				, m_complete_handler(complethandler)
			{
			}

			void operator()()
			{
				m_accepted_socket_store.reset(new ClientSocket(m_socket_creator()));
				m_socket.async_accept(*m_accepted_socket_store, std::move(*this));
			}

			void operator()(boost::system::error_code ec)
			{
				if ( m_accept_handler(std::move(*m_accepted_socket_store), ec))
				{
					(*this)();
				}else
				{
					m_complete_handler();
				}
			}
		};


		template<class Socket, class ClientSocket, class SocketCreator, class AcceptHandler, class CompleteHandler>
			async_accept_op<Socket, ClientSocket, SocketCreator, AcceptHandler, CompleteHandler>
		make_async_accept_op(Socket& sock, SocketCreator&& sc, AcceptHandler&& accepthandler, CompleteHandler&& complethandler)
		{
			return async_accept_op<Socket, ClientSocket, SocketCreator, AcceptHandler, CompleteHandler>(
				sock, std::forward<SocketCreator>(sc), std::forward<AcceptHandler>(accepthandler), std::forward<CompleteHandler>(complethandler)
			);
		}
	}

/*
 *  async_accept 接受一个已经被设置为 listen 模式的 Socket, 一个 SocketCreator 用于创建
 *  接收新链接的 Socket, 一个 AcceptHandler 用于处理 Accept , 最后一个 CompleteHandler
 *  会在退出 accept 循环的时候调用
 */
template<class Socket, class SocketCreator, class AcceptHandler, class CompleteHandler>
void async_accept(Socket& sock, SocketCreator&& sc, AcceptHandler&& accepthandler, CompleteHandler&& complethandler)
{
	typedef decltype(sc()) ClientSocket;

	detail::make_async_accept_op<Socket, ClientSocket, SocketCreator, AcceptHandler, CompleteHandler>
		(sock, std::forward<SocketCreator>(sc), std::forward<AcceptHandler>(accepthandler), std::forward<CompleteHandler>(complethandler))();
}


/*
 * listen 接受一个新创建的 accept socket, 并将其设置为 listen 模式
 */

template<class Socket, class Logger>
bool listen(Socket& sock, const std::string& service, Logger&& logger)
{
	boost::system::error_code ignore_ec;

	boost::asio::ip::tcp::resolver resolver(sock.get_io_service());

	boost::asio::ip::tcp::resolver::query query(service);
	boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, ignore_ec);
	if (ignore_ec)
	{
 		logger << "Socket DNS resolve failed for bind address: " << ignore_ec.message();
		return false;
	}
	boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
	sock.open(endpoint.protocol(), ignore_ec);
	if (ignore_ec)
	{
		logger << "Socket open protocol failed: " << ignore_ec.message();
		return false;
	}
	sock.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ignore_ec);
	if (ignore_ec)
	{
		logger << "Socket set option failed: " << ignore_ec.message();
		return false;
	}
	sock.bind(endpoint, ignore_ec);
	if (ignore_ec)
	{
		logger << "Socket bind failed: " << ignore_ec.message();
		return false;
	}
	sock.listen(boost::asio::socket_base::max_connections, ignore_ec);
	if (ignore_ec)
	{
		logger << "Socket listen failed: " << ignore_ec.message();
		return false;
	}
	return true;
}

template<class Socket>
bool listen(Socket& sock, const std::string& service)
{
	return listen(sock, service, std::cerr);
}

} // namespace acceptor

