
#incluce <acceptor/acceptor.hpp>

#include <boost/asio.hpp>

struct connection : std::enable_shared_from_this<connection>
{
	connection(boost::asio::ip::tcp::socket&& sock)
		: m_socket(sock)
	{}

	void start()
	{
		m_socket.async_read_some(boost::asio::buffer(m_buf), std::bind(&connection::handle_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	}

private:

	void handle_read(boost::system::error_code ec, std::size_t bytes)
	{
		if (!ec)
			m_socket.async_write_some(boost::asio::buffer(m_buf, bytes), std::bind(&connection::handle_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

	}

	void handle_write(boost::system::error_code ec, std::size_t bytes)
	{
		if (!ec)
			m_socket.async_read_some(boost::asio::buffer(m_buf), std::bind(&connection::handle_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
	}

private:

	boost::asio::ip::tcp::socket m_socket;

	std::array<char, 512> m_buf;

};

std::shared_ptr<connection> create_connection(boost::asio::ip::tcp::socket&& sock)
{
	return std::make_shared<connection>(std::move(sock));
}

int main()
{
	boost::asio::io_service io;
	boost::asio::ip::tcp::acceptor accept_socket(io);

	acceptor::async_accept(accept_socket,
		[io](){return boost::asio::ip::tcp::socket(io);},
		[io](boost::asio::ip::tcp::socket&& sock, boost::system::error_code error) -> bool
		{
			create_connection(std::move(sock))->start();
			return true;
		},
		[](){}
	);

	io.run();
}
