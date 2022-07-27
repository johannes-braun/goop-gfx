//////////////////////////////////////////////////////////
// THIS FILE IS GENERATED BY CMAKE, PLEASE DO NOT CHANGE!
// Prefix: ${LIBRARY_PREFIX}
// Prefix (uppercase): ${LIBRARY_PREFIX_UPPER}
// Shader: ${SHADER_NAME}
// Shader (C): ${SHADER_NAME_C}
// Namespace: ${SHADER_NAMESPACE}
//////////////////////////////////////////////////////////
#include <${LIBRARY_PREFIX}/${SHADER_HEADER}.h>
#include <secure_string.hpp>

namespace ${SHADER_NAMESPACE} {
constexpr static security::secure_const_string ${SHADER_NAME}_data(R"(${EMPLACEMENT_HINT})");

${LIBRARY_PREFIX_UPPER}_EXPORT std::string_view ${SHADER_NAME}()
{
    static auto const string = ${SHADER_NAME}_data.str();
    return string;
}
}

#ifdef __cplusplus
extern "C" {
#endif
${LIBRARY_PREFIX_UPPER}_EXPORT char const* ${SHADER_NAME_C}()
{
    return ${SHADER_NAMESPACE}::${SHADER_NAME}().data();
}

${LIBRARY_PREFIX_UPPER}_EXPORT unsigned long long ${SHADER_NAME_C}_length()
{
    return ${SHADER_NAMESPACE}::${SHADER_NAME}().size();
}
#ifdef __cplusplus
}
#endif