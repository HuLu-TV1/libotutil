#pragma once 

#include <functional>
#include <string>
#include <cstdio>
#include <memory>

#define CAPI

#define BUILT_DELETE_FUNCTION(decl) decl = delete;
#define DISALLOW_COPY(TypeName) \
    BUILT_DELETE_FUNCTION(TypeName(const TypeName&))
#define DISALLOW_ASSIGN(TypeName) \
    BUILT_DELETE_FUNCTION(TypeName& operator=(const TypeName&))
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    DISALLOW_COPY(TypeName) \
    DISALLOW_ASSIGN(TypeName)
#define DISALLOW_MOVE(TypeName) \
    BUILT_DELETE_FUNCTION(TypeName(TypeName&&))
#define DISALLOW_MOVE_ASSIGN(TypeName) \
    BUILT_DELETE_FUNCTION(TypeName& operator=(TypeName&&))
#define DISALLOW_MOVE_AND_ASSING(TypeName) \
    DISALLOW_MOVE(TypeName) \
    DISALLOW_MOVE_ASSIGN(TypeName)
#define DISALLOW_COPY_AND_MOVE(TypeName) \
    DISALLOW_COPY_AND_ASSIGN(TypeName) \
    DISALLOW_MOVE_AND_ASSING(TypeName)


// struct file_event_handlers {
//     file_event_handlers()
//         : before_open(nullptr)
//         , after_open(nullptr)
//         , before_close(nullptr)
//         , after_close(nullptr)
//     {}

//     std::function<void(const std::string& filename)> before_open;
//     std::function<void(const std::string& filename, std::FILE* file_stream)> after_open;
//     std::function<void(const std::string& filename, std::FILE* file_stream)> before_close;
//     std::function<void(const std::string& filename)> after_close;
// };