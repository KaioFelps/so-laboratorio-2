#include "command.h"
#include <optional>
#include <unistd.h>

using namespace std;

const string &Command::get_program() const { return this->arguments_.front(); }

const vector<char *> Command::get_constant_arguments() const
{
  auto arguments_as_c_strings = vector<char *>();

  for (auto &program : this->arguments_)
  {
    arguments_as_c_strings.push_back(const_cast<char *>(program.data()));
  }

  return arguments_as_c_strings;
}

int Command::run()
{
  const auto program = this->get_program();
  auto arguments = this->get_constant_arguments();

  this->state_ = Command::State::Running;

  return execvp(program.data(), arguments.data());
}

const bool Command::is_background_task() const { return this->is_background_task_; }

const optional<reference_wrapper<Command>> Command::get_chained_command() const
{
  if (this->state_ == State::Failed && this->failure_chained_command.has_value())
  {
    return make_optional(ref(**this->failure_chained_command));
  }

  else if (this->state_ == State::SuccessfullyTerminated &&
           this->success_chained_command.has_value())
  {
    return make_optional(ref(**this->success_chained_command));
  }

  return nullopt;
}

void Command::mark_as_finished(bool successfully)
{
  if (successfully) this->state_ = Command::State::SuccessfullyTerminated;
  this->state_ = Command::State::Failed;
}

void Command::chain_on_failure(std::shared_ptr<Command> command)
{
  this->failure_chained_command = make_optional(command);
}

void Command::chain_on_success(std::shared_ptr<Command> command)
{
  this->success_chained_command = make_optional(command);
}
