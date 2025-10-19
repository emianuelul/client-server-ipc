#pragma once
#include "include_libs.h"

class CommandException : public std::runtime_error {
 public:
  explicit CommandException(const char* message)
      : std::runtime_error(message) {}
};

class AuthException : public std::runtime_error {
 public:
  explicit AuthException(const char* message) : std::runtime_error(message) {}
};

class InvalidCommandException : public std::runtime_error {
 public:
  explicit InvalidCommandException(const char* message)
      : std::runtime_error(message) {}
};

class ForkException : public std::runtime_error {
 public:
  explicit ForkException(const char* message) : std::runtime_error(message) {}
};

class FileException : public std::runtime_error {
 public:
  explicit FileException(const char* message) : std::runtime_error(message) {}
};

class PipeException : public std::runtime_error {
 public:
  explicit PipeException(const char* message) : std::runtime_error(message) {}
};