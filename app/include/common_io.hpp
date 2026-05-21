#pragma once

#include <boost/asio/io_context.hpp>

// // 声明:io 是 thread_local 全局变量，外部定义
extern thread_local boost::asio::io_context io;
