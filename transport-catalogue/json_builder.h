#pragma once

#include <stack>
#include "json.h"

namespace json {
	class DictItemContext;
	class ArrayItemContext;
	class KeyItemContext;

	class Builder {
	public:
		DictItemContext StartDict();
		ArrayItemContext StartArray();
		Builder& EndDict();
		Builder& EndArray();
		KeyItemContext Key(std::string);
		Builder& Value(Node&& node);
		Node Build() const;
	private:
		Node root_;
		std::stack<Node*> node_stack_;
	};

	class BaseContext {
	public:
		BaseContext(Builder& builder)
			:builder_(builder) {
		}
		DictItemContext StartDict();
		ArrayItemContext StartArray();
		Builder& EndDict();
		Builder& EndArray();
		KeyItemContext Key(std::string str);
		Builder& Value(Node&& node);
		[[nodiscard]]Node Build() const;
	private:
		Builder& builder_;
	};

	class DictItemContext : public BaseContext {
	public:
		DictItemContext StartDict() = delete;
		ArrayItemContext StartArray() = delete;
		Builder& EndArray() = delete;
		Builder& Value(Node&& node) = delete;
		[[nodiscard]] Node Build() const = delete;
	};

	class ArrayItemContext : public BaseContext {
	public:
		Builder& EndDict() = delete;
		KeyItemContext Key(std::string) = delete;
		[[nodiscard]] Node Build() const = delete;
	};

	class KeyItemContext : public BaseContext {
	public:
		Builder& EndDict() = delete;
		Builder& EndArray() = delete;
		KeyItemContext Key(std::string) = delete;
		[[nodiscard]] Node Build() const = delete;
	};
}

