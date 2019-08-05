#include "UDPHandler.hpp"

UDPHandler::UDPHandler(std::string ip, int sendPort, int receivePort)
 : sendPort{sendPort}
{
	socket.open(ip::udp::v4(), error);
	send_endpoint = ip::udp::endpoint(ip::address::from_string(ip), sendPort);
	receive_endpoint = ip::udp::endpoint(ip::udp::v4(), receivePort);
	socket.bind(receive_endpoint, error);
	service.run(error);

	receiveThread = std::thread(&UDPHandler::receive, this);
}

void UDPHandler::send(std::string message)
{
	socket.send_to(buffer(message), send_endpoint, 0, error);
}

void UDPHandler::receive()
{
	while (true)
	{
		boost::array<char, 5000> recv_buf;
		size_t len = socket.receive_from(boost::asio::buffer(recv_buf), send_endpoint);

		receivedMessage = std::string(recv_buf.data()).substr(0, len);
		
		send_endpoint = ip::udp::endpoint(send_endpoint.address(), sendPort);

		send("received");
	}
}

void UDPHandler::close()
{
	socket.close();
}

std::string UDPHandler::getMessage()
{
	return std::string(receivedMessage);
}

void UDPHandler::clearMessage()
{
	receivedMessage.clear();
}

std::string UDPHandler::getIP()
{
	return send_endpoint.address().to_string();
}

int UDPHandler::getSendPort()
{
	return send_endpoint.port();
}

int UDPHandler::getReceivePort()
{
	return receive_endpoint.port();
}