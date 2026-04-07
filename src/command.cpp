#include "command.h"
#include <algorithm>
#include <optional>
#include <ranges>
#include <unistd.h>

using namespace std;

Command::Command(const std::vector<std::string> &args) : arguments_(args) {}

const string &Command::get_program() const { return this->arguments_.front(); }

const vector<char *> Command::get_constant_arguments() const
{
  auto arguments_as_c_strings = vector<char *>();

  for (auto &program : this->arguments_)
  {
    arguments_as_c_strings.push_back(const_cast<char *>(program.data()));
  }

  if (arguments_as_c_strings.back() != nullptr) arguments_as_c_strings.push_back(nullptr);

  return arguments_as_c_strings;
}

int Command::run()
{
  const auto program = this->get_program();
  auto arguments = this->get_constant_arguments();

  this->status_ = Command::Status::Running;

  return execvp(program.data(), arguments.data());
}

const bool Command::is_background_task() const { return this->is_background_task_; }

const optional<reference_wrapper<Command>> Command::get_chained_command() const
{
  const auto shall_chain_failure_command = this->status_ == Status::Failed;
  const auto shall_chain_success_command =
      this->status_ == Status::SuccessfullyTerminated || this->is_background_task_;

  if (shall_chain_failure_command && this->failure_chained_command_.has_value())
  {
    return make_optional(ref(**this->failure_chained_command_));
  }

  else if (shall_chain_success_command && this->success_chained_command_.has_value())
  {
    return make_optional(ref(**this->success_chained_command_));
  }

  return nullopt;
}

void Command::mark_as_finished(bool successfully)
{
  this->status_ = successfully ? Command::Status::SuccessfullyTerminated : Command::Status::Failed;
}

void Command::chain_on_failure(std::shared_ptr<Command> command)
{
  this->failure_chained_command_ = make_optional(command);
}

void Command::chain_on_success(std::shared_ptr<Command> command)
{
  this->success_chained_command_ = make_optional(command);
}

void Command::turn_into_background_task() { this->is_background_task_ = true; }

const std::vector<std::string> &Command::get_arguments() const { return arguments_; }

const std::optional<std::shared_ptr<Command>> &Command::get_failure_chained_command() const
{
  return this->failure_chained_command_;
}

const std::optional<std::shared_ptr<Command>> &Command::get_success_chained_command() const
{
  return success_chained_command_;
}

Command::Status Command::get_status() const { return this->status_; }

std::string Command::get_command_string() const
{
  // Aparentemente, C++ agora também tem pipes kkk
  auto command_string = this->arguments_ | views::join_with(' ') | ranges::to<string>();
  return command_string;
}