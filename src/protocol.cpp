#include "protocol.h"
#include "json_serializer.h"

namespace minirpc
{
    
Serializer *Protocol::m_serializer = new JsonSerializer();

} // namespace minirpc
