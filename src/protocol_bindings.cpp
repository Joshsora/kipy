#include <string>
#include <iostream>

#include <pybind11/pybind11.h>

#include <ki/dml/types.h>
#include <ki/dml/Record.h>
#include <ki/protocol/exception.h>
#include <ki/protocol/dml/Message.h>
#include <ki/protocol/dml/MessageBuilder.h>
#include <ki/protocol/dml/MessageTemplate.h>
#include <ki/protocol/dml/MessageModule.h>
#include <ki/protocol/dml/MessageManager.h>
#include <ki/protocol/net/PacketHeader.h>
#include <ki/protocol/net/Participant.h>
#include <ki/protocol/net/Session.h>
#include <ki/protocol/control/Opcode.h>
#include <ki/protocol/control/ServerHello.h>
#include <ki/protocol/control/Ping.h>
#include <ki/protocol/control/ClientHello.h>

#define DEF_SET_FIELD_VALUE_METHOD(NAME, TYPE)    \
    .def(NAME,                                    \
        &MessageBuilder::set_field_value<TYPE>,   \
        py::arg("name"),                          \
        py::arg("value"),                         \
        py::return_value_policy::take_ownership)

namespace py = pybind11;

class TrampolineParticipant : public ki::protocol::net::Participant
{
public:
    using ki::protocol::net::Participant::Participant;
    void send_packet_data(const char *data, const size_t size) override
    {
        PYBIND11_OVERLOAD_PURE(
            void,
            ki::protocol::net::Participant,
            send_packet_data,
            data, size);
    }
    void close() override
    {
        PYBIND11_OVERLOAD_PURE(
            void,
            ki::protocol::net::Participant,
            close, );
    }
};

class TrampolineSession : public ki::protocol::net::Session
{
public:
    using ki::protocol::net::Session::Session;
    void send_packet_data(const char *data, const size_t size) override
    {
        PYBIND11_OVERLOAD_PURE(
            void,
            ki::protocol::net::Session,
            send_packet_data,
            data, size);
    }
    void close() override
    {
        PYBIND11_OVERLOAD_PURE(
            void,
            ki::protocol::net::Session,
            close, );
    }
};

class PublicistParticipant : public ki::protocol::net::Participant
{
public:
    using ki::protocol::net::Participant::send_packet_data;
    using ki::protocol::net::Participant::close;
};

PYBIND11_MODULE(protocol, m)
{
    using namespace ki::protocol;

    // Exceptions
    py::register_exception<runtime_error>(m, "ProtocolRuntimeError");
    py::register_exception<parse_error>(m, "ProtocolParseError");
    py::register_exception<value_error>(m, "ProtocolValueError");

    using namespace ki::protocol::dml;

    // dml Submodule
    py::module m_dml = m.def_submodule("dml");

    // Message Class
    py::class_<Message>(m_dml, "Message")
        .def(py::init<uint8_t, uint8_t>(),
            py::arg("service_id") = 0,
            py::arg("type") = 0)
        // Properties
        .def_property("service_id",
            &Message::get_service_id,
            &Message::set_service_id,
            py::return_value_policy::copy)
        .def_property("type",
            &Message::get_type,
            &Message::set_type,
            py::return_value_policy::copy)
        .def_property("record",
            static_cast<ki::dml::Record *(Message::*)()>(&Message::get_record),
            &Message::set_record,
            py::return_value_policy::reference)
        // Read-only Properties
        .def_property_readonly("size",
            &Message::get_size,
            py::return_value_policy::copy)
        // Methods
        .def("use_template_record",
            &Message::use_template_record,
            py::arg("record"))
        // Extensions
        .def("to_bytes",
            [](const Message &self)
            {
                std::ostringstream oss;
                self.write_to(oss);
                return py::bytes(oss.str());
            },
            py::return_value_policy::copy)
        .def("from_bytes",
            [](Message &self, std::string data)
            {
                std::istringstream iss(data);
                self.read_from(iss);
            },
            py::arg("data"));

    // MessageBuilder Class
    py::class_<MessageBuilder>(m_dml, "MessageBuilder")
        .def(py::init<uint8_t, uint8_t>(),
            py::arg("service_id") = 0,
            py::arg("type") = 0)
        // Read-only Properties
        .def_property_readonly("message",
            &MessageBuilder::get_message,
            py::return_value_policy::take_ownership)
        // Methods
        .def("set_service_id",
            &MessageBuilder::set_service_id,
            py::arg("service_id"),
            py::return_value_policy::take_ownership)
        .def("set_message_type",
            &MessageBuilder::set_message_type,
            py::arg("type"),
            py::return_value_policy::take_ownership)
        .def("use_template_record",
            &MessageBuilder::use_template_record,
            py::arg("record"),
            py::return_value_policy::take_ownership)
        // set_field_value Methods
        DEF_SET_FIELD_VALUE_METHOD("set_byt_field_value", ki::dml::BYT)
        DEF_SET_FIELD_VALUE_METHOD("set_ubyt_field_value", ki::dml::UBYT)
        DEF_SET_FIELD_VALUE_METHOD("set_shrt_field_value", ki::dml::SHRT)
        DEF_SET_FIELD_VALUE_METHOD("set_ushrt_field_value", ki::dml::USHRT)
        DEF_SET_FIELD_VALUE_METHOD("set_int_field_value", ki::dml::INT)
        DEF_SET_FIELD_VALUE_METHOD("set_uint_field_value", ki::dml::UINT)
        DEF_SET_FIELD_VALUE_METHOD("set_str_field_value", ki::dml::STR)
        DEF_SET_FIELD_VALUE_METHOD("set_wstr_field_value", ki::dml::WSTR)
        DEF_SET_FIELD_VALUE_METHOD("set_flt_field_value", ki::dml::FLT)
        DEF_SET_FIELD_VALUE_METHOD("set_dbl_field_value", ki::dml::DBL)
        DEF_SET_FIELD_VALUE_METHOD("set_gid_field_value", ki::dml::GID);

    // MessageTemplate Class
    py::class_<MessageTemplate>(m_dml, "MessageTemplate")
        .def(py::init<std::string, uint8_t, uint8_t, ki::dml::Record *>(),
            py::arg("name"), py::arg("type"),
            py::arg("service_id"), py::arg("record"))
        // Properties
        .def_property("name",
            &MessageTemplate::get_name,
            &MessageTemplate::set_name,
            py::return_value_policy::copy)
        .def_property("type",
            &MessageTemplate::get_type,
            &MessageTemplate::set_type,
            py::return_value_policy::copy)
        .def_property("service_id",
            &MessageTemplate::get_service_id,
            &MessageTemplate::set_service_id,
            py::return_value_policy::copy)
        .def_property("record",
            &MessageTemplate::get_record,
            &MessageTemplate::set_record,
            py::return_value_policy::reference)
        // Methods
        .def("build_message",
            &MessageTemplate::build_message,
            py::return_value_policy::take_ownership);

    // MessageModule Class
    py::class_<MessageModule>(m_dml, "MessageModule")
        .def(py::init<uint8_t, std::string>(),
            py::arg("service_id") = 0,
            py::arg("protocol_type") = "")
        // Descriptors
        .def("__getitem__",
            [](const MessageModule &self, uint8_t key)
            {
                auto *message_template = self.get_message_template(key);
                if (message_template)
                    return message_template;
                throw py::key_error("MessageTemplate with type " + std::to_string(key) + " does not exist");
            },
            py::arg("key"),
            py::return_value_policy::reference)
        .def("__getitem__",
            [](const MessageModule &self, std::string key)
            {
                auto *message_template = self.get_message_template(key);
                if (message_template)
                    return message_template;
                throw py::key_error("MessageTemplate with name '" + key + "' does not exist");
            },
            py::arg("key"),
            py::return_value_policy::reference)
        // Properties
        .def_property("service_id",
            &MessageModule::get_service_id,
            &MessageModule::set_service_id,
            py::return_value_policy::copy)
        .def_property("protocol_type",
            &MessageModule::get_protocol_type,
            &MessageModule::set_protocol_type,
            py::return_value_policy::copy)
        .def_property("protocol_description",
            &MessageModule::get_protocol_desription,
            &MessageModule::set_protocol_description,
            py::return_value_policy::copy)
        // Methods
        .def("add_message_template",
            &MessageModule::add_message_template,
            py::return_value_policy::reference)
        .def("get_message_template",
            static_cast<const MessageTemplate *(MessageModule::*)(uint8_t) const>(
                &MessageModule::get_message_template),
            py::arg("type"),
            py::return_value_policy::reference)
        .def("get_message_template",
            static_cast<const MessageTemplate *(MessageModule::*)(std::string) const>(
                &MessageModule::get_message_template),
            py::arg("name"),
            py::return_value_policy::reference)
        .def("sort_lookup", &MessageModule::sort_lookup)
        .def("build_message",
            static_cast<MessageBuilder &(MessageModule::*)(uint8_t) const>(
                &MessageModule::build_message),
            py::arg("message_type"),
            py::return_value_policy::take_ownership)
        .def("build_message",
            static_cast<MessageBuilder &(MessageModule::*)(std::string) const>(
                &MessageModule::build_message),
            py::arg("message_name"),
            py::return_value_policy::take_ownership);

    // MessageManager Class
    py::class_<MessageManager>(m_dml, "MessageManager")
        .def(py::init<>())
        // Methods
        .def("load_module",
            &MessageManager::load_module,
            py::arg("filepath"),
            py::return_value_policy::reference)
        .def("get_module",
            static_cast<const MessageModule *(MessageManager::*)(uint8_t) const>(
                &MessageManager::get_module),
            py::arg("service_id"),
            py::return_value_policy::reference)
        .def("get_module",
            static_cast<const MessageModule *(MessageManager::*)(const std::string &) const>(
                &MessageManager::get_module),
            py::arg("protocol_type"),
            py::return_value_policy::reference)
        .def("build_message",
            static_cast<MessageBuilder &(MessageManager::*)(uint8_t, uint8_t) const>(
                &MessageManager::build_message),
            py::arg("service_id"),
            py::arg("message_type"),
            py::return_value_policy::take_ownership)
        .def("build_message",
            static_cast<MessageBuilder &(MessageManager::*)(uint8_t, const std::string &) const>(
                &MessageManager::build_message),
            py::arg("service_id"),
            py::arg("message_name"),
            py::return_value_policy::take_ownership)
        .def("build_message",
            static_cast<MessageBuilder &(MessageManager::*)(const std::string &, uint8_t) const>(
                &MessageManager::build_message),
            py::arg("protocol_type"),
            py::arg("message_type"),
            py::return_value_policy::take_ownership)
        .def("build_message",
            static_cast<MessageBuilder &(MessageManager::*)(const std::string &, const std::string &) const>(
                &MessageManager::build_message),
            py::arg("protocol_type"),
            py::arg("message_name"),
            py::return_value_policy::take_ownership)
        // Extensions
        .def("message_from_bytes",
            [](MessageManager &self, std::string data)
            {
                std::istringstream iss(data);
                return self.message_from_binary(iss);
            },
            py::arg("data"));

    // dml Submodule (end)

    using namespace ki::protocol::net;

    // net Submodule
    py::module m_net = m.def_submodule("net");

    // PacketHeader Class
    py::class_<PacketHeader>(m_net, "PacketHeader")
        .def(py::init<bool, uint8_t>(),
            py::arg("control") = false,
            py::arg("opcode") = 0)
        // Properties
        .def_property("control",
            &PacketHeader::is_control,
            &PacketHeader::set_control,
            py::return_value_policy::copy)
        .def_property("opcode",
            &PacketHeader::get_opcode,
            &PacketHeader::set_opcode,
            py::return_value_policy::copy)
        // Read-only Properties
        .def_property_readonly("size",
            &PacketHeader::get_size,
            py::return_value_policy::copy)
        // Extensions
        .def("to_bytes",
            [](const PacketHeader &self)
            {
                std::ostringstream oss;
                self.write_to(oss);
                return py::bytes(oss.str());
            },
            py::return_value_policy::copy)
        .def("from_bytes",
            [](PacketHeader &self, std::string data)
            {
                std::istringstream iss(data);
                self.read_from(iss);
            },
            py::arg("data"));

    // ReceiveState Enum
    py::enum_<ReceiveState>(m_net, "ReceiveState")
        .value("WAITING_FOR_START_SIGNAL", ReceiveState::WAITING_FOR_START_SIGNAL)
        .value("WAITING_FOR_LENGTH", ReceiveState::WAITING_FOR_LENGTH)
        .value("WAITING_FOR_PACKET", ReceiveState::WAITING_FOR_PACKET);

    // ParticipantType Enum
    py::enum_<ParticipantType>(m_net, "ParticipantType")
        .value("SERVER", ParticipantType::SERVER)
        .value("CLIENT", ParticipantType::CLIENT);

    // Participant Class
    py::class_<Participant, TrampolineParticipant> participant(m_net, "Participant");
    participant.def(py::init<ParticipantType>(), py::arg("type"))
        // Properties
        .def_property("type",
            &Participant::get_type,
            &Participant::set_type,
            py::return_value_policy::copy)
        .def_property("maximum_packet_size",
            &Participant::get_maximum_packet_size,
            &Participant::set_maximum_packet_size,
            py::return_value_policy::copy)
        // Virtual Methods
        .def("send_packet_data", &PublicistParticipant::send_packet_data)
        .def("close", &PublicistParticipant::close);

    // Session Class
    py::class_<Session, TrampolineSession>(m_net, "Session", participant)
        .def(py::init<ParticipantType, uint16_t>(),
            py::arg("type"), py::arg("id"))
        // Properties
        .def_property("access_level",
            &Session::get_access_level,
            &Session::set_access_level,
            py::return_value_policy::copy)
        // Read-only Properties
        .def_property_readonly("id",
            &Session::get_id,
            py::return_value_policy::copy)
        .def_property_readonly("established",
            &Session::is_established,
            py::return_value_policy::copy)
        .def_property_readonly("latency",
            &Session::get_latency,
            py::return_value_policy::copy)
        .def_property_readonly("alive",
            &Session::is_alive,
            py::return_value_policy::copy);

    // net Submodule (end)

    using namespace ki::protocol::control;

    // control Submodule
    py::module m_control = m.def_submodule("control");

    // Opcode Enum
    py::enum_<Opcode>(m_control, "Opcode")
        .value("SERVER_HELLO", Opcode::SERVER_HELLO)
        .value("UDP_HELLO", Opcode::UDP_HELLO)
        .value("PING", Opcode::PING)
        .value("PING_RSP", Opcode::PING_RSP)
        .value("CLIENT_HELLO", Opcode::CLIENT_HELLO);

    // ServerHello Class
    py::class_<ServerHello>(m_control, "ServerHello")
        .def(py::init<uint16_t, int32_t, uint32_t>(),
            py::arg("session_id") = 0,
            py::arg("timestamp") = 0,
            py::arg("milliseconds") = 0)
        // Properties
        .def_property("session_id",
            &ServerHello::get_session_id,
            &ServerHello::set_session_id,
            py::return_value_policy::copy)
        .def_property("timestamp",
            &ServerHello::get_timestamp,
            &ServerHello::set_timestamp,
            py::return_value_policy::copy)
        .def_property("milliseconds",
            &ServerHello::get_milliseconds,
            &ServerHello::set_milliseconds,
            py::return_value_policy::copy)
        // Read-only Properties
        .def_property_readonly("size",
            &ServerHello::get_size,
            py::return_value_policy::copy)
        // Extensions
        .def("to_bytes",
            [](const ServerHello &self)
            {
                std::ostringstream oss;
                self.write_to(oss);
                return py::bytes(oss.str());
            },
            py::return_value_policy::copy)
        .def("from_bytes",
            [](ServerHello &self, std::string data)
            {
                std::istringstream iss(data);
                self.read_from(iss);
            },
            py::arg("data"));

    // Ping Class
    py::class_<Ping>(m_control, "Ping")
        .def(py::init<uint16_t, uint16_t, uint8_t>(),
            py::arg("session_id") = 0,
            py::arg("milliseconds") = 0,
            py::arg("minutes") = 0)
        // Properties
        .def_property("session_id",
            &Ping::get_session_id,
            &Ping::set_session_id,
            py::return_value_policy::copy)
        .def_property("milliseconds",
            &Ping::get_milliseconds,
            &Ping::set_milliseconds,
            py::return_value_policy::copy)
        .def_property("minutes",
            &Ping::get_minutes,
            &Ping::set_minutes,
            py::return_value_policy::copy)
        // Read-only Properties
        .def_property_readonly("size",
            &Ping::get_size,
            py::return_value_policy::copy)
        // Extensions
        .def("to_bytes",
            [](const Ping &self)
            {
                std::ostringstream oss;
                self.write_to(oss);
                return py::bytes(oss.str());
            },
            py::return_value_policy::copy)
        .def("from_bytes",
            [](Ping &self, std::string data)
            {
                std::istringstream iss(data);
                self.read_from(iss);
            },
            py::arg("data"));

    // ClientHello Class
    py::class_<ClientHello>(m_control, "ClientHello")
        .def(py::init<uint16_t, int32_t, uint32_t>(),
            py::arg("session_id") = 0,
            py::arg("timestamp") = 0,
            py::arg("milliseconds") = 0)
        // Properties
        .def_property("session_id",
            &ClientHello::get_session_id,
            &ClientHello::set_session_id,
            py::return_value_policy::copy)
        .def_property("timestamp",
            &ClientHello::get_timestamp,
            &ClientHello::set_timestamp,
            py::return_value_policy::copy)
        .def_property("milliseconds",
            &ClientHello::get_milliseconds,
            &ClientHello::set_milliseconds,
            py::return_value_policy::copy)
        // Read-only Properties
        .def_property_readonly("size",
            &ClientHello::get_size,
            py::return_value_policy::copy)
        // Extensions
        .def("to_bytes",
            [](const ClientHello &self)
            {
                std::ostringstream oss;
                self.write_to(oss);
                return py::bytes(oss.str());
            },
            py::return_value_policy::copy)
        .def("from_bytes",
            [](ClientHello &self, std::string data)
            {
                std::istringstream iss(data);
                self.read_from(iss);
            },
            py::arg("data"));

    // control Submodule (end)
}
