#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <string>
#include <vector>
#include <iostream>

#include "Thread.hpp"

using namespace boost::asio;

class UDPHandler : public Thread
{
private:
	int sendPort;

	io_service service;
	ip::udp::socket socket{service};
	ip::udp::endpoint receive_endpoint, send_endpoint;

	std::string receivedMessage;

	void handleReceive(const boost::system::error_code &error, std::size_t bytes_transferred);
	void threadFunction();

public:
	UDPHandler(std::string ip, int sendPort, int receivePort);
	~UDPHandler();
	void send(std::string message);
	void stop() override;
	std::string getMessage();
	void clearMessage();
	std::string getIP();
	int getSendPort();
	int getReceivePort();
};
