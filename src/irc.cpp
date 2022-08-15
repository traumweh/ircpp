/*
 * SPDX-FileCopyrightText: 2022 traumweh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ircpp/irc.hpp>

namespace traumweh::ircpp {

Irc::Irc(std::string host, conFunc onConnection, msgFunc onMessage,
         errFunc onError)
    : host(host), onCon(onConnection), onMsg(onMessage), onErr(onError),
      webSocket() {
  webSocket.setUrl(host);

  webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr &msg) {
    if (msg->type == ix::WebSocketMessageType::Open) {
      this->onCon();
    } else if (msg->type == ix::WebSocketMessageType::Message) {
      this->splitBatch(msg->str);
    } else if (msg->type == ix::WebSocketMessageType::Error) {
      this->onErr(msg->errorInfo.reason);
    }
  });

  webSocket.start();
}

auto Irc::authenticate(std::string pass, std::string nick,
                       std::optional<std::string> caps) -> void {
  if (caps.has_value())
    webSocket.send("CAP REQ :" + caps.value());

  webSocket.send("PASS " + pass);
  webSocket.send("NICK " + nick);
}

auto Irc::join(std::string channel) -> void {
  webSocket.send("JOIN #" + channel);
}

auto Irc::splitBatch(const std::string batch) -> void {
  auto rawMessages = splitAtSep(batch, "\r\n");

  for (const auto &rawMessage : rawMessages) {
    auto parsed = parseMessage(rawMessage);

    if (!parsed.has_value()) // not a valid irc message
      continue;

    IRCMessage msg = parsed.value();

    if (msg.command == "PING")             // recv "PING :token"
      if (msg.params.trailing.has_value()) // has token
        webSocket.send("PONG :" +
                       msg.params.trailing.value()); // send "PONG :token"

    onMsg(msg); // call msg handler; including PING.
  }
}

auto Irc::parseMessage(const std::string &msg) -> std::optional<IRCMessage> {
  std::string raw = msg.substr(0, msg.find('\r')); // trim CRLF

  IRCMessage message = {raw, {}, {}, {}, {}};
  std::size_t position = 0;
  std::size_t nextspace = 0;

  // Parse Tags as key-val-map; if only key is present use `true` as val
  if (raw[0] == '@') {
    nextspace = raw.find(' ');

    if (nextspace == raw.npos)
      return {};

    std::vector<std::string> rawTags =
        splitAtSep(raw.substr(1, nextspace), ";");

    for (const auto &tag : rawTags) {
      auto pair = splitAtSep(tag, "=");

      if (pair.size() == 1)
        message.tags[pair[0]] = true;
      else
        message.tags[pair[0]] = pair[1];
    }

    position = nextspace + 1;
  }

  // skip extra spaces
  while (raw[position] == ' ')
    position++;

  // parse source
  if (raw[position] == ':') {
    nextspace = raw.find(' ', position);

    if (nextspace == raw.npos)
      return {}; // invalid irc-message

    message.prefix = raw.substr(position + 1, nextspace - (position + 1));
    position = nextspace + 1;

    while (raw[position] == ' ')
      position++;
  }

  // parse command
  nextspace = raw.find(' ', position);

  if (nextspace == raw.npos) {
    if (position < raw.npos) {
      message.command = raw.substr(position);
      return message; // irc message without parameters
    }
    return {}; // invalid irc message
  }

  // command if irc message with parameters
  message.command = raw.substr(position, nextspace - position);
  position = nextspace + 1;

  // skip extra spaces
  while (raw[position] == ' ')
    position++;

  // parse params
  while (position < raw.npos) {
    nextspace = raw.find(' ', position);

    // trailing parameter, everything after colon is a single parameter
    if (raw[position] == ':') {
      message.params.trailing = raw.substr(position + 1);
      break;
    }

    if (nextspace != raw.npos) {
      std::string param = raw.substr(position, nextspace - position);

      if (message.params.middle.has_value())
        message.params.middle.value().push_back(param);
      else
        message.params.middle = {param};

      position = nextspace + 1;

      while (raw[position] == ' ')
        position++;
    } else {
      std::string param = raw.substr(position);

      if (message.params.middle.has_value())
        message.params.middle.value().push_back(param);
      else
        message.params.middle = {param};

      break;
    }
  }

  return message;
}

auto Irc::write(const std::string &msg) -> void { webSocket.send(msg); }

auto Irc::write(const IRCMessage &msg) -> void {
  if (msg.raw.has_value()) {
    webSocket.send(msg.raw.value());
    return;
  }

  std::string message;

  if (msg.tags.size() > 0) {
    message += "@";

    for (const auto &pair : msg.tags) {
      message += pair.first;

      if (std::holds_alternative<std::string>(pair.second))
        message += "=" + std::get<std::string>(pair.second) + ";";
    }

    message[message.size() - 1] = ' ';
  }

  if (msg.prefix.has_value())
    message += ":" + msg.prefix.value() + " ";

  message += msg.command + " ";

  if (msg.params.middle.has_value())
    for (const auto &param : msg.params.middle.value())
      message += param + " ";

  if (msg.params.trailing.has_value())
    message += ":" + msg.params.trailing.value();

  webSocket.send(message);
}

auto Irc::splitAtSep(std::string str, std::string sep)
    -> std::vector<std::string> {
  std::vector<std::string> out;

  std::size_t start = 0, end = str.find(sep);

  while (end != str.npos) {
    out.push_back(str.substr(start, end - start));
    start = end + sep.size();
    end = str.find(sep, start);
  }

  if (start != str.npos)
    out.push_back(str.substr(start));

  return out;
}

} // namespace traumweh::ircpp
