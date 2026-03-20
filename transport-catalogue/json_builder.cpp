#include "json_builder.h"
#include "json.h"

namespace json {
	DictItemContext Builder::StartDict(){
		if (root_.IsNull()) {
			root_ = Dict{};
			node_stack_.push(&root_);
		}
		else if (!node_stack_.empty()) {
			Node* node_stack_top = node_stack_.top();
			if (node_stack_top->IsNull()) {
				*node_stack_top = Dict{};
			}
			else if (node_stack_top->IsArray()) {
				node_stack_top->AsArray().emplace_back(Dict{});
				node_stack_.push(&node_stack_top->AsArray().back());
			}
			else {
				throw std::logic_error("Incorrect using StartDict");
			}
		}
		else {
			throw std::logic_error("Incorrect using StartDict");
		}
		return DictItemContext(*this);
	}

	ArrayItemContext Builder::StartArray() {
		if (root_.IsNull()) {
			root_ = Array{};
			node_stack_.push(&root_);
		}
		else if (!node_stack_.empty()) {
			Node* node_stack_top = node_stack_.top();
			if (node_stack_top->IsNull()) {
				*node_stack_top = Array{};
			}
			else if (node_stack_top->IsArray()) {
				node_stack_top->AsArray().emplace_back(Array{});
				node_stack_.push(&node_stack_top->AsArray().back());
			}
			else {
				throw std::logic_error("Incorrect using StartArray");
			}
		}
		else {
			throw std::logic_error("Incorrect using StartArray");
		}
		return ArrayItemContext(*this);
	}

	Builder& Builder::EndDict() {
		if (!node_stack_.empty()) {
			Node* node_stack_top = node_stack_.top();
			if (node_stack_top->IsDict()) {
				node_stack_.pop();
			}
			else {
				throw std::logic_error("Incorrect using EndDict");
			}
		}
		else {
			throw std::logic_error("Incorrect using EndDict");
		}
		return *this;
	}

	Builder& Builder::EndArray() {
		if (!node_stack_.empty()) {
			Node* node_stack_top = node_stack_.top();
			if (node_stack_top->IsArray()) {
				node_stack_.pop();
			}
			else {
				throw std::logic_error("Incorrect using EndArray");
			}
		}
		else {
			throw std::logic_error("Incorrect using EndArray");
		}
		return *this;
	}

	KeyItemContext Builder::Key(std::string str) {
		if (!node_stack_.empty()) {
			Node* node_stack_top = node_stack_.top();
			if (node_stack_top->IsDict()) {
				node_stack_top->AsDict()[str] = Node{};
				node_stack_.push(&node_stack_top->AsDict()[str]);
			}
			else {
				throw std::logic_error("Incorrect using Key");
			}
		}
		else {
			throw std::logic_error("Incorrect using Key");
		}
		return KeyItemContext(*this);
	}

	Builder& Builder::Value(Node&& node) {
		if (root_.IsNull()) {
			root_ = std::move(node);
		}
		else if (!node_stack_.empty()) {
			Node* node_stack_top = node_stack_.top();
			if (node_stack_top->IsNull()) {
				*node_stack_top = std::move(node);
				node_stack_.pop();
			}
			else if (node_stack_top->IsArray()) {
				node_stack_top->AsArray().emplace_back(std::move(node));
			}
			else {
				throw std::logic_error("Incorrect using Value");
			}
		}
		else {
			throw std::logic_error("Incorrect using Value");
		}
		return *this;
	}

	Node Builder::Build() const {
		if (!root_.IsNull() && node_stack_.empty()) {
			return root_;
		}
		else {
			throw std::logic_error("Incorrect using Build");
		}
	}

	DictItemContext BaseContext::StartDict() {
		return builder_.StartDict();
	}

	ArrayItemContext BaseContext::StartArray() {
		return builder_.StartArray();
	}

	Builder& BaseContext::EndDict() {
		return builder_.EndDict();
	}

	Builder& BaseContext::EndArray() {
		return builder_.EndArray();
	}

	KeyItemContext BaseContext::Key(std::string str) {
		return builder_.Key(str);
	}

	Builder& BaseContext::Value(Node&& node) {
		return builder_.Value(std::move(node));
	}

	Node BaseContext::Build() const {
		return builder_.Build();
	}
} // namespace json


