#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <thread>

using namespace boost::asio;

class UDPHandler
{
private:
	int sendPort;

	io_service service;
	ip::udp::socket socket{service};
	ip::udp::endpoint receive_endpoint, send_endpoint;

	std::string receivedMessage;
	std::thread receiveThread;

	void receive();
	void handleReceive(const boost::system::error_code &error, std::size_t bytes_transferred);

public:
	UDPHandler(std::string ip, int sendPort, int receivePort);
	void send(std::string message);
	void close();
	std::string getMessage();
	void clearMessage();
	std::string getIP();
	int getSendPort();
	int getReceivePort();
};
