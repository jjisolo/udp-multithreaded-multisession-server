#ifndef __OZZY_COMPONENT_INFORMATIVE__
#define __OZZY_COMPONENT_INFORMATIVE__

namespace Ozzy::Component
{
    class Informative
    {
        protected:
            Informative(const std::string logger_name)
                : m_logger_name("[Ozzy::" + logger_name + "] ")
            {
            }

        protected:
            std::string m_logger_name;
    };
}

#endif // __OZZY_COMPONENT_INFORMATIVE__