#include "UDPHandler.hpp"

UDPHandler::UDPHandler(std::string ip, int sendPort, int receivePort)
	: ip{ip}, sendPort{sendPort}
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
	std::string header{};
	if (message.length() < 10)
		header.append("000");
	else if (message.length() < 100)
		header.append("00");
	else if (message.length() < 1000)
		header.append("0");
	message = header + std::to_string(message.length()) + message;

	socket.send_to(buffer(message, bufferSize), send_endpoint, 0, error);
}

void UDPHandler::receive()
{
	while (true)
	{
		char buf[bufferSize];
		socket.receive(buffer(buf, bufferSize), 0, error);

		std::string message{buf};
		int length{std::stoi(message.substr(0, headerSize))};

		receivedMessage = message.substr(headerSize, length);

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
	return ip;
}

int UDPHandler::getSendPort()
{
	return sendPort;
}

int UDPHandler::getReceivePort()
{
	return receivePort;
}