#include <string>
#include <iostream>

#include <pybind11/pybind11.h>

#include <ki/dml/types.h>
#include <ki/dml/Record.h>
#include <ki/protocol/exception.h>
#include <ki/protocol/dml/Message.h>
#include <ki/protocol/dml/MessageTemplate.h>
#include <ki/protocol/dml/MessageModule.h>
#include <ki/protocol/dml/MessageManager.h>
#include <ki/protocol/net/PacketHeader.h>
#include <ki/protocol/net/Session.h>
#include <ki/protocol/net/ServerSession.h>
#include <ki/protocol/net/ClientSession.h>
#include <ki/protocol/net/DMLSession.h>
#include <ki/protocol/net/ServerDMLSession.h>
#include <ki/protocol/net/ClientDMLSession.h>
#include <ki/protocol/control/Opcode.h>
#include <ki/protocol/control/SessionOffer.h>
#include <ki/protocol/control/ServerKeepAlive.h>
#include <ki/protocol/control/ClientKeepAlive.h>
#include <ki/protocol/control/SessionAccept.h>

// Disable inheritance via dominance warning
#if _MSC_VER
#pragma warning(disable: 4250)
#endif

#define DEF_SET_FIELD_VALUE_METHOD(NAME, TYPE)    \
    .def(NAME,                                    \
        &MessageBuilder::set_field_value<TYPE>,   \
        py::arg("name"),                          \
        py::arg("value"),                         \
        py::return_value_policy::take_ownership)

#define DEF_TO_BYTES_EXTENSION(SELF)      \
    .def("to_bytes",                      \
        [](const SELF &self)              \
        {                                 \
            std::ostringstream oss;       \
            self.write_to(oss);           \
            return py::bytes(oss.str());  \
        },                                \
        py::return_value_policy::copy)
#define DEF_FROM_BYTES_EXTENSION(SELF)     \
    .def("from_bytes",                     \
        [](SELF &self, std::string data)   \
        {                                  \
            std::istringstream iss(data);  \
            self.read_from(iss);           \
        },                                 \
        py::arg("data"))

namespace py = pybind11;

class PySession : public ki::protocol::net::Session
{
public:
    PySession(const uint16_t id)
        : Session(id) {}
    bool is_alive() const override
    {
        PYBIND11_OVERLOAD_PURE(
            bool, ki::protocol::net::Session,
            is_alive, );
    }
    void on_invalid_packet() override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::Session,
            on_invalid_packet, );
    }
    void on_control_message(const ki::protocol::net::PacketHeader &header) override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::Session,
            on_control_message, header);
    }
    void on_application_message(const ki::protocol::net::PacketHeader &header) override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::Session,
            on_application_message, header);
    }
    void send_packet_data(const char *data, const size_t size) override
    {
        PYBIND11_OVERLOAD_PURE(
            void, ki::protocol::net::Session,
            send_packet_data, py::bytes(data, size), size);
    }
    void close(ki::protocol::net::SessionCloseErrorCode error) override
    {
        PYBIND11_OVERLOAD_PURE(
            void, ki::protocol::net::Session,
            close, error);
    }
};

class PyServerSession : public ki::protocol::net::ServerSession
{
public:
    PyServerSession(const uint16_t id)
        : Session(id), ServerSession(id) {}
    void on_invalid_packet() override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::ServerSession,
            on_invalid_packet, );
    }
    void on_application_message(const ki::protocol::net::PacketHeader &header) override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::ServerSession,
            on_application_message, header);
    }
    void send_packet_data(const char *data, const size_t size) override
    {
        PYBIND11_OVERLOAD_PURE(
            void, ki::protocol::net::ServerSession,
            send_packet_data, py::bytes(data, size), size);
    }
    void close(ki::protocol::net::SessionCloseErrorCode error) override
    {
        PYBIND11_OVERLOAD_PURE(
            void, ki::protocol::net::ServerSession,
            close, error);
    }
    void on_established() override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::ServerSession,
            on_established, );
    }
};

class PyClientSession : public ki::protocol::net::ClientSession
{
public:
    PyClientSession(const uint16_t id)
        : Session(id), ClientSession(id) {}
    void on_invalid_packet() override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::ClientSession,
            on_invalid_packet, );
    }
    void on_application_message(const ki::protocol::net::PacketHeader &header) override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::ClientSession,
            on_application_message, header);
    }
    void send_packet_data(const char *data, const size_t size) override
    {
        PYBIND11_OVERLOAD_PURE(
            void, ki::protocol::net::ClientSession,
            send_packet_data, py::bytes(data, size), size);
    }
    void close(ki::protocol::net::SessionCloseErrorCode error) override
    {
        PYBIND11_OVERLOAD_PURE(
            void, ki::protocol::net::ClientSession,
            close, error);
    }
    void on_established() override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::ClientSession,
            on_established, );
    }
};

class PyDMLSession : public ki::protocol::net::DMLSession
{
public:
    PyDMLSession(const uint16_t id, const ki::protocol::dml::MessageManager &manager)
        : Session(id), DMLSession(id, manager) {}
    bool is_alive() const override
    {
        PYBIND11_OVERLOAD_PURE(
            bool, ki::protocol::net::DMLSession,
            is_alive, );
    }
    void on_invalid_packet() override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::DMLSession,
            on_invalid_packet, );
    }
    void on_control_message(const ki::protocol::net::PacketHeader &header) override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::DMLSession,
            on_control_message, header);
    }
    void send_packet_data(const char *data, const size_t size) override
    {
        PYBIND11_OVERLOAD_PURE(
            void, ki::protocol::net::DMLSession,
            send_packet_data, py::bytes(data, size), size);
    }
    void close(ki::protocol::net::SessionCloseErrorCode error) override
    {
        PYBIND11_OVERLOAD_PURE(
            void, ki::protocol::net::DMLSession,
            close, error);
    }
    void on_message(const ki::protocol::dml::Message *message) override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::DMLSession,
            on_message, message);
    }
    void on_invalid_message(ki::protocol::net::InvalidDMLMessageErrorCode error) override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::DMLSession,
            on_invalid_message, error);
    }
};

class PyServerDMLSession : public ki::protocol::net::ServerDMLSession
{
public:
    PyServerDMLSession(const uint16_t id, const ki::protocol::dml::MessageManager &manager)
        : Session(id), ServerDMLSession(id, manager) {}
    void on_invalid_packet() override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::ServerDMLSession,
            on_invalid_packet, );
    }
    void send_packet_data(const char *data, const size_t size) override
    {
        PYBIND11_OVERLOAD_PURE(
            void, ki::protocol::net::ServerDMLSession,
            send_packet_data, py::bytes(data, size), size);
    }
    void close(ki::protocol::net::SessionCloseErrorCode error) override
    {
        PYBIND11_OVERLOAD_PURE(
            void, ki::protocol::net::ServerDMLSession,
            close, error);
    }
    void on_established() override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::ServerDMLSession,
            on_established, );
    }
    void on_message(const ki::protocol::dml::Message *message) override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::ServerDMLSession,
            on_message, message);
    }
    void on_invalid_message(ki::protocol::net::InvalidDMLMessageErrorCode error) override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::ServerDMLSession,
            on_invalid_message, error);
    }
};

class PyClientDMLSession : public ki::protocol::net::ClientDMLSession
{
public:
    PyClientDMLSession(const uint16_t id, const ki::protocol::dml::MessageManager &manager)
        : Session(id), ClientDMLSession(id, manager) {}
    void on_invalid_packet() override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::ClientDMLSession,
            on_invalid_packet, );
    }
    void send_packet_data(const char *data, const size_t size) override
    {
        PYBIND11_OVERLOAD_PURE(
            void, ki::protocol::net::ClientDMLSession,
            send_packet_data, py::bytes(data, size), size);
    }
    void close(ki::protocol::net::SessionCloseErrorCode error) override
    {
        PYBIND11_OVERLOAD_PURE(
            void, ki::protocol::net::ClientDMLSession,
            close, error);
    }
    void on_established() override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::ClientDMLSession,
            on_established, );
    }
    void on_message(const ki::protocol::dml::Message *message) override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::ClientDMLSession,
            on_message, message);
    }
    void on_invalid_message(ki::protocol::net::InvalidDMLMessageErrorCode error) override
    {
        PYBIND11_OVERLOAD(
            void, ki::protocol::net::ClientDMLSession,
            on_invalid_message, error);
    }
};

class PublicistSession: public ki::protocol::net::Session
{
public:
    using ki::protocol::net::Session::process_data;
    using ki::protocol::net::Session::on_invalid_packet;
    using ki::protocol::net::Session::on_control_message;
    using ki::protocol::net::Session::on_application_message;
    using ki::protocol::net::Session::send_packet_data;
    using ki::protocol::net::Session::close;
};

class PublicistServerSession : public ki::protocol::net::ServerSession
{
public:
    using ki::protocol::net::ServerSession::on_connected;
    using ki::protocol::net::ServerSession::on_established;
};

class PublicistClientSession : public ki::protocol::net::ClientSession
{
public:
    using ki::protocol::net::ClientSession::on_connected;
    using ki::protocol::net::ClientSession::on_established;
};

class PublicistDMLSession : public ki::protocol::net::DMLSession
{
public:
    using ki::protocol::net::DMLSession::on_message;
    using ki::protocol::net::DMLSession::on_invalid_message;
};

PYBIND11_MODULE(protocol, m)
{
    using namespace ki::protocol;

    // Exceptions
    py::register_exception<runtime_error>(m, "ProtocolRuntimeError");
    py::register_exception<parse_error>(m, "ProtocolParseError");
    py::register_exception<value_error>(m, "ProtocolValueError");

    using namespace ki::protocol::dml;

    // Submodule: dml

    py::module m_dml = m.def_submodule("dml");

    // Class: Message
    py::class_<Message>(m_dml, "Message")

        // Initializer
        .def(py::init<const MessageTemplate *>(),
            py::arg("message_template ") = nullptr)

        // Descriptor: __getitem__
        .def("__getitem__",
            [](const Message &self, std::string key)
            {
                auto *field = self.get_field(key);
                if (field)
                    return field;
                throw py::key_error("Field with name " + key + " does not exist");
            },
            py::arg("key"), py::return_value_policy::reference)

        // Property: template
        .def_property("template",
            &Message::get_template,
            &Message::set_template, py::return_value_policy::copy)

        // Property: record (read-only)
        .def_property_readonly("record",
            static_cast<ki::dml::Record *(Message::*)()>(&Message::get_record),
            py::return_value_policy::reference_internal)
        // def_property_readonly: service_id (read-only)
        .def_property_readonly("service_id", &Message::get_service_id,
            py::return_value_policy::copy)
        // Property: type (read-only)
        .def_property_readonly("type", &Message::get_type,
            py::return_value_policy::copy)
        // Property: message_size (read-only)
        .def_property_readonly("message_size", &Message::get_message_size,
            py::return_value_policy::copy)
        // Property: handler (read-only)
        .def_property_readonly("handler", &Message::get_handler,
            py::return_value_policy::copy)
        // Property: access_level (read-only)
        .def_property_readonly("access_level", &Message::get_access_level,
            py::return_value_policy::copy)
        // Property: size (read-only)
        .def_property_readonly("size", &Message::get_size,
            py::return_value_policy::copy)

        // Extension: to_bytes()
        DEF_TO_BYTES_EXTENSION(Message)
        // Extension: from_bytes()
        DEF_FROM_BYTES_EXTENSION(Message)
        ;

    // Class: MessageTemplate
    py::class_<MessageTemplate>(m_dml, "MessageTemplate")

        // Initializer
        .def(py::init<std::string, uint8_t, uint8_t, ki::dml::Record *>(),
            py::arg("name"),
            py::arg("type"),
            py::arg("service_id"),
            py::arg("record"))

        // Property: name
        .def_property("name",
            &MessageTemplate::get_name,
            &MessageTemplate::set_name, py::return_value_policy::copy)
        // Property: type
        .def_property("type",
            &MessageTemplate::get_type,
            &MessageTemplate::set_type, py::return_value_policy::copy)
        // Property: service_id
        .def_property("service_id",
            &MessageTemplate::get_service_id,
            &MessageTemplate::set_service_id, py::return_value_policy::copy)
        // Property: handler
        .def_property("handler",
            &MessageTemplate::get_handler,
            &MessageTemplate::set_handler, py::return_value_policy::copy)
        // Property: access_level
        .def_property("access_level",
            &MessageTemplate::get_access_level,
            &MessageTemplate::set_access_level, py::return_value_policy::copy)
        // Property: record
        .def_property("record",
            &MessageTemplate::get_record,
            &MessageTemplate::set_record, py::return_value_policy::reference_internal)

        // Method: create_message()
        .def("create_message", &MessageTemplate::create_message,
            py::return_value_policy::take_ownership)
        ;

    // Class: MessageModule
    py::class_<MessageModule>(m_dml, "MessageModule")

        // Initializer
        .def(py::init<uint8_t, std::string>(),
            py::arg("service_id") = 0,
            py::arg("protocol_type") = "")

        // Descriptor: __getitem__
        .def("__getitem__",
            [](const MessageModule &self, uint8_t key)
            {
                auto *message_template = self.get_message_template(key);
                if (message_template)
                    return message_template;
                throw py::key_error("MessageTemplate with type " + std::to_string(key) + " does not exist");
            },
            py::arg("key"), py::return_value_policy::reference)
        // Descriptor: __getitem__
        .def("__getitem__",
            [](const MessageModule &self, std::string key)
            {
                auto *message_template = self.get_message_template(key);
                if (message_template)
                    return message_template;
                throw py::key_error("MessageTemplate with name '" + key + "' does not exist");
            },
            py::arg("key"), py::return_value_policy::reference)

        // Property: service_id
        .def_property("service_id",
            &MessageModule::get_service_id,
            &MessageModule::set_service_id, py::return_value_policy::copy)
        // Property: protocol_type
        .def_property("protocol_type",
            &MessageModule::get_protocol_type,
            &MessageModule::set_protocol_type, py::return_value_policy::copy)
        // Property: protocol_description
        .def_property("protocol_description",
            &MessageModule::get_protocol_desription,
            &MessageModule::set_protocol_description, py::return_value_policy::copy)

        // Method: add_message_template()
        .def("add_message_template", &MessageModule::add_message_template,
            py::return_value_policy::reference)
        // Method: sort_lookup()
        .def("sort_lookup", &MessageModule::sort_lookup)
        // Method: create_message()
        .def("create_message",
            static_cast<Message *(MessageModule::*)(uint8_t) const>(
                &MessageModule::create_message),
            py::arg("message_type"), py::return_value_policy::take_ownership)
        // Method: create_message()
        .def("create_message",
            static_cast<Message *(MessageModule::*)(std::string) const>(
                &MessageModule::create_message),
            py::arg("message_name"), py::return_value_policy::take_ownership)
        ;

    // Class: MessageManager
    py::class_<MessageManager>(m_dml, "MessageManager")

        // Initializer
        .def(py::init<>())

        // Descriptor: __getitem__
        .def("__getitem__",
            [](const MessageManager &self, uint8_t key)
            {
                auto *module = self.get_module(key);
                if (module)
                    return module;
                throw py::key_error("MessageModule with service ID " + std::to_string(key) + " does not exist");
            },
            py::arg("key"), py::return_value_policy::reference)
        // Descriptor: __getitem__
        .def("__getitem__",
            [](const MessageManager &self, const std::string &key)
            {
                auto *module = self.get_module(key);
                if (module)
                    return module;
                throw py::key_error("MessageModule with protocol type '" + key + "' does not exist");
            },
            py::arg("key"), py::return_value_policy::reference)

        // Method: load_module()
        .def("load_module", &MessageManager::load_module,
            py::arg("filepath"), py::return_value_policy::reference)
        // Method: create_message()
        .def("create_message",
            static_cast<Message *(MessageManager::*)(uint8_t, uint8_t) const>(
                &MessageManager::create_message),
            py::arg("service_id"),
            py::arg("message_type"), py::return_value_policy::take_ownership)
        // Method: create_message()
        .def("create_message",
            static_cast<Message *(MessageManager::*)(uint8_t, const std::string &) const>(
                &MessageManager::create_message),
            py::arg("service_id"),
            py::arg("message_name"), py::return_value_policy::take_ownership)
        // Method: create_message()
        .def("create_message",
            static_cast<Message *(MessageManager::*)(const std::string &, uint8_t) const>(
                &MessageManager::create_message),
            py::arg("protocol_type"),
            py::arg("message_type"), py::return_value_policy::take_ownership)
        // Method: create_message()
        .def("create_message",
            static_cast<Message *(MessageManager::*)(const std::string &, const std::string &) const>(
                &MessageManager::create_message),
            py::arg("protocol_type"),
            py::arg("message_name"), py::return_value_policy::take_ownership)

        // Extension: message_from_bytes()
        .def("message_from_bytes",
            [](MessageManager &self, std::string data)
            {
                std::istringstream iss(data);
                return self.message_from_binary(iss);
            },
            py::arg("data"))
        ;

    // Submodule: dml (end)

    using namespace ki::protocol::net;

    // Submodule: net

    py::module m_net = m.def_submodule("net");

    // Class: PacketHeader
    py::class_<PacketHeader>(m_net, "PacketHeader")

        // Initializer
        .def(py::init<bool, uint8_t>(),
            py::arg("control") = false,
            py::arg("opcode") = 0)

        // Property: control
        .def_property("control",
            &PacketHeader::is_control,
            &PacketHeader::set_control, py::return_value_policy::copy)
        // Property: opcode
        .def_property("opcode",
            &PacketHeader::get_opcode,
            &PacketHeader::set_opcode, py::return_value_policy::copy)

        // Property: size (read-only)
        .def_property_readonly("size", &PacketHeader::get_size,
            py::return_value_policy::copy)

        // Extension: to_bytes()
        DEF_TO_BYTES_EXTENSION(PacketHeader)
        // Extension: from_bytes()
        DEF_FROM_BYTES_EXTENSION(PacketHeader)
        ;

    // Enum: ReceiveState
    py::enum_<ReceiveState>(m_net, "ReceiveState")
        .value("WAITING_FOR_START_SIGNAL", ReceiveState::WAITING_FOR_START_SIGNAL)
        .value("WAITING_FOR_LENGTH", ReceiveState::WAITING_FOR_LENGTH)
        .value("WAITING_FOR_PACKET", ReceiveState::WAITING_FOR_PACKET)
        ;

    // Enum: SessionCloseErrorCode
    py::enum_<SessionCloseErrorCode>(m_net, "SessionCloseErrorCode")
        .value("NONE", SessionCloseErrorCode::NONE)
        .value("APPLICATION_ERROR", SessionCloseErrorCode::APPLICATION_ERROR)
        .value("INVALID_FRAMING_START_SIGNAL", SessionCloseErrorCode::INVALID_FRAMING_START_SIGNAL)
        .value("INVALID_FRAMING_SIZE_EXCEEDS_MAXIMUM", SessionCloseErrorCode::INVALID_FRAMING_SIZE_EXCEEDS_MAXIMUM)
        .value("UNHANDLED_CONTROL_MESSAGE", SessionCloseErrorCode::UNHANDLED_CONTROL_MESSAGE)
        .value("UNHANDLED_APPLICATION_MESSAGE", SessionCloseErrorCode::UNHANDLED_APPLICATION_MESSAGE)
        .value("INVALID_MESSAGE", SessionCloseErrorCode::INVALID_MESSAGE)
        .value("SESSION_OFFER_TIMED_OUT", SessionCloseErrorCode::SESSION_OFFER_TIMED_OUT)
        .value("SESSION_DIED", SessionCloseErrorCode::SESSION_DIED)
        ;

    // Class: Session
    py::class_<Session, PySession>(m_net, "Session")

        // Initializer
        .def(py::init<uint16_t>(),
            py::arg("id") = 0)

        // Property: maximum_packet_size
        .def_property("maximum_packet_size",
            &Session::get_maximum_packet_size,
            &Session::set_maximum_packet_size, py::return_value_policy::copy)
        // Property: access_level
        .def_property("access_level",
            &Session::get_access_level,
            &Session::set_access_level, py::return_value_policy::copy)

        // Property: id (read-only)
        .def_property_readonly("id", &Session::get_id,
            py::return_value_policy::copy)
        // Property: established (read-only)
        .def_property_readonly("established", &Session::is_established,
            py::return_value_policy::copy)
        // Property: latency (read-only)
        .def_property_readonly("latency", &Session::get_latency,
            py::return_value_policy::copy)
        // Property: alive (read-only)
        .def_property_readonly("alive", &Session::is_alive,
            py::return_value_policy::copy)

        // Method: send_packet()
        .def("send_packet", &Session::send_packet,
            py::arg("is_control"),
            py::arg("opcode"),
            py::arg("data"))

        // Method: process_data() (protected)
        .def("process_data", &PublicistSession::process_data,
            py::arg("data"),
            py::arg("size"))

        // Method: on_invalid_packet() (protected, virtual)
        .def("on_invalid_packet", &PublicistSession::on_invalid_packet)
        // Method: on_control_message() (protected, virtual)
        .def("on_control_message", &PublicistSession::on_control_message,
            py::arg("header"))
        // Method: on_application_message() (protected, virtual)
        .def("on_application_message", &PublicistSession::on_application_message,
            py::arg("header"))

        // Method: send_packet_data() (protected, pure virtual)
        .def("send_packet_data", &PublicistSession::send_packet_data,
            py::arg("data"),
            py::arg("size"))
        // Method: close() (protected, pure virtual)
        .def("close", &PublicistSession::close,
            py::arg("error"))
        ;

    // Class: ServerSession
    py::class_<ServerSession, Session, PyServerSession>(
        m_net, "ServerSession", py::multiple_inheritance())

        // Initializer
        .def(py::init<uint16_t>(),
            py::arg("id"))

        // Method: send_keep_alive()
        .def("send_keep_alive", &ServerSession::send_keep_alive,
            py::arg("milliseconds_since_startup"))

        // Method: on_connected() (protected)
        .def("on_connected", &PublicistServerSession::on_connected)

        // Method: on_established() (protected, virtual)
        .def("on_established", &PublicistServerSession::on_established)
        ;

    // Class: ClientSession
    py::class_<ClientSession, Session, PyClientSession>(
        m_net, "ClientSession", py::multiple_inheritance())

        // Initializer
        .def(py::init<uint16_t>(),
            py::arg("id"))

        // Method: send_keep_alive()
        .def("send_keep_alive", &ClientSession::send_keep_alive)

        // Method: on_connected() (protected)
        .def("on_connected", &PublicistClientSession::on_connected)

        // Method: on_established() (protected, virtual)
        .def("on_established", &PublicistClientSession::on_established)
        ;

    // Enum: InvalidDMLMessageErrorCode
    py::enum_<InvalidDMLMessageErrorCode>(m_net, "InvalidDMLMessageErrorCode")
        .value("NONE", InvalidDMLMessageErrorCode::NONE)
        .value("UNKNOWN", InvalidDMLMessageErrorCode::UNKNOWN)
        .value("INVALID_HEADER_DATA", InvalidDMLMessageErrorCode::INVALID_HEADER_DATA)
        .value("INVALID_MESSAGE_DATA", InvalidDMLMessageErrorCode::INVALID_MESSAGE_DATA)
        .value("INVALID_SERVICE", InvalidDMLMessageErrorCode::INVALID_SERVICE)
        .value("INVALID_MESSAGE_TYPE", InvalidDMLMessageErrorCode::INVALID_MESSAGE_TYPE)
        .value("INSUFFICIENT_ACCESS", InvalidDMLMessageErrorCode::INSUFFICIENT_ACCESS)
        ;

    // Class: DMLSession
    py::class_<DMLSession, Session, PyDMLSession>(
        m_net, "DMLSession", py::multiple_inheritance())

        // Initializer
        .def(py::init<uint16_t, const ki::protocol::dml::MessageManager &>(),
            py::arg("id"),
            py::arg("manager"))

        // Property: manager (read-only)
        .def_property_readonly("manager", &DMLSession::get_manager,
            py::return_value_policy::reference_internal)

        // Method: send_message()
        .def("send_message", &DMLSession::send_message,
            py::arg("message"))

        // Method: on_message() (protected, virtual)
        .def("on_message", &PublicistDMLSession::on_message,
            py::arg("message"))

        // Method: on_invalid_message() (protected, virtual)
        .def("on_invalid_message", &PublicistDMLSession::on_invalid_message)
        ;

    // Class: ServerDMLSession
    py::class_<ServerDMLSession, ServerSession, DMLSession, PyServerDMLSession>(
        m_net, "ServerDMLSession", py::multiple_inheritance())

        // Initializer
        .def(py::init<uint16_t, const ki::protocol::dml::MessageManager &>(),
            py::arg("id"),
            py::arg("manager"))
        ;

    // Class: ClientDMLSession
    py::class_<ClientDMLSession, ClientSession, DMLSession, PyClientDMLSession>(
        m_net, "ClientDMLSession", py::multiple_inheritance())

        // Initializer
        .def(py::init<uint16_t, const ki::protocol::dml::MessageManager &>(),
            py::arg("id"),
            py::arg("manager"))
        ;

    // Submodule: net (end)

    using namespace ki::protocol::control;

    // Submodule: control

    py::module m_control = m.def_submodule("control");

    // Enum: Opcode
    py::enum_<Opcode>(m_control, "Opcode")
        .value("NONE", Opcode::NONE)
        .value("SESSION_OFFER", Opcode::SESSION_OFFER)
        .value("UDP_HELLO", Opcode::UDP_HELLO)
        .value("KEEP_ALIVE", Opcode::KEEP_ALIVE)
        .value("KEEP_ALIVE_RSP", Opcode::KEEP_ALIVE_RSP)
        .value("SESSION_ACCEPT", Opcode::SESSION_ACCEPT)
        ;

    // Class: SessionOffer
    py::class_<SessionOffer>(m_control, "SessionOffer")

        // Initializer
        .def(py::init<uint16_t, int32_t, uint32_t>(),
            py::arg("session_id") = 0,
            py::arg("timestamp") = 0,
            py::arg("milliseconds") = 0)

        // Property: session_id
        .def_property("session_id",
            &SessionOffer::get_session_id,
            &SessionOffer::set_session_id, py::return_value_policy::copy)
        // Property: timestamp
        .def_property("timestamp",
            &SessionOffer::get_timestamp,
            &SessionOffer::set_timestamp, py::return_value_policy::copy)
        // Property: milliseconds
        .def_property("milliseconds",
            &SessionOffer::get_milliseconds,
            &SessionOffer::set_milliseconds, py::return_value_policy::copy)

        // Property: size (read-only)
        .def_property_readonly("size", &SessionOffer::get_size,
            py::return_value_policy::copy)

        // Extension: to_bytes()
        DEF_TO_BYTES_EXTENSION(SessionOffer)
        // Extension: from_bytes()
        DEF_FROM_BYTES_EXTENSION(SessionOffer)
        ;

    // Class: ServerKeepAlive
    py::class_<ServerKeepAlive>(m_control, "ServerKeepAlive")

        // Initializer
        .def(py::init<uint32_t>(),
            py::arg("timestamp") = 0)

        // Property: timestamp
        .def_property("timestamp",
            &ServerKeepAlive::get_timestamp,
            &ServerKeepAlive::set_timestamp, py::return_value_policy::copy)

        // Property: size (read-only)
        .def_property_readonly("size", &ServerKeepAlive::get_size,
            py::return_value_policy::copy)

        // Extension: to_bytes()
        DEF_TO_BYTES_EXTENSION(ServerKeepAlive)
        // Extension: from_bytes()
        DEF_FROM_BYTES_EXTENSION(ServerKeepAlive)
        ;

    // Class: ClientKeepAlive
    py::class_<ClientKeepAlive>(m_control, "ClientKeepAlive")

        // Initializer
        .def(py::init<uint16_t, uint16_t, uint16_t>(),
            py::arg("session_id") = 0,
            py::arg("milliseconds") = 0,
            py::arg("minutes") = 0)

        // Property: session_id
        .def_property("session_id",
            &ClientKeepAlive::get_session_id,
            &ClientKeepAlive::set_session_id, py::return_value_policy::copy)
        // Property: milliseconds
        .def_property("milliseconds",
            &ClientKeepAlive::get_milliseconds,
            &ClientKeepAlive::set_milliseconds, py::return_value_policy::copy)
        // Property: minutes
        .def_property("minutes",
            &ClientKeepAlive::get_minutes,
            &ClientKeepAlive::set_minutes, py::return_value_policy::copy)

        // Property: size (read-only)
        .def_property_readonly("size", &ClientKeepAlive::get_size,
            py::return_value_policy::copy)

        // Extension: to_bytes()
        DEF_TO_BYTES_EXTENSION(ClientKeepAlive)
        // Extension: from_bytes()
        DEF_FROM_BYTES_EXTENSION(ClientKeepAlive)
        ;

    // Class: SessionAccept
    py::class_<SessionAccept>(m_control, "SessionAccept")

        // Initializer
        .def(py::init<uint16_t, int32_t, uint32_t>(),
            py::arg("session_id") = 0,
            py::arg("timestamp") = 0,
            py::arg("milliseconds") = 0)

        // Property: session_id
        .def_property("session_id",
            &SessionAccept::get_session_id,
            &SessionAccept::set_session_id, py::return_value_policy::copy)
        // Property: timestamp
        .def_property("timestamp",
            &SessionAccept::get_timestamp,
            &SessionAccept::set_timestamp, py::return_value_policy::copy)
        // Property: milliseconds
        .def_property("milliseconds",
            &SessionAccept::get_milliseconds,
            &SessionAccept::set_milliseconds, py::return_value_policy::copy)

        // Property: size (read-only)
        .def_property_readonly("size", &SessionAccept::get_size,
            py::return_value_policy::copy)

        // Extension: to_bytes()
        DEF_TO_BYTES_EXTENSION(SessionAccept)
        // Extension: from_bytes()
        DEF_FROM_BYTES_EXTENSION(SessionAccept)
        ;

    // Submodule: control (end)
}

// Re-enable inheritance via dominance warning
#if _MSC_VER
#pragma warning(default: 4250)
#endif
