#pragma once

// Um templatezinho pra não precisar criar `Visitor`s específicos pra cada `std::variant`.
template <class... Ts> struct match : Ts...
{
  using Ts::operator()...;
};
