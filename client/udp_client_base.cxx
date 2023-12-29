#include "udp_client_base.h"

namespace Ozzy::Base
{
    UdpClientBase::~UdpClientBase()
    {
        m_socket.close();
    }
}
