#pragma once

#include <string>
#include <vector>

#include "utils.h"

namespace utils {

    class Command {
    public:
        std::string const & command() const { return cmd_; }
        std::vector<std::string> const & args() const { return args_; }
        std::string const & workingDirectory() const { return wd_; }

    private:

    }; // utils::Command


} // namespace utils