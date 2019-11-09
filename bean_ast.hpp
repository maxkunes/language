#pragma once
#include <iostream>
#include <memory>
#include <cstdint>
#include <map>
#include "bean_object.hpp"
#include "tokenizer.hpp"


class bean_state;
class ast;

class ast_builder
{
public:
	static void handle_parentheses(token_iterator& iterator)
	{
		while (iterator.get_size() >= 2 && iterator.first().get_type() == token_type::lparen && iterator.last().get_type() == token_type::rparen)
		{
			int parenCount = 0;

			token_iterator paren_iterator(iterator.get_tokens());

			while (paren_iterator.next().is_valid())
			{
				auto parenToken = paren_iterator.here();

				if (parenToken.get_type() == token_type::lparen)
				{
					parenCount++;
				}
				else if (parenToken.get_type() == token_type::rparen)
				{
					parenCount--;
					if (paren_iterator.get_index() != iterator.get_size() - 1 && parenCount == 0)
					{
						// We are not at the end of the expression but we have no open parentheses, therefore the start and end parentheses are not linked.
						goto escapeParen;
					}
					else if (parenCount == 0)
					{
						// Last element.
						iterator.pop_front();
						iterator.pop_end();
					}
				}
			}
		}
	escapeParen:
		return;
	}

	static std::shared_ptr<ast> parse(const token_array& tokens, bean_state& state);
};

using bean_objects = std::vector<std::shared_ptr<bean_object>>;
using bean_function_decl = std::function<bean_objects(bean_objects&)>;


class bean_function
{
public:
	explicit bean_function(bean_function_decl& fun, const std::string name)
	{
		fun_ = std::move(fun);
		name_ = name;
		//parameter_types_ = parameter_types;
	}

	bean_objects operator()(bean_objects& params) const
	{
		return fun_(params);
	}

	[[nodiscard]] std::string get_name() const
	{
		return name_;
	}
	
private:
	bean_function_decl fun_;
	std::string name_;
	//std::vector<bean_object_type_descriptor> parameter_types_;
};


class bean_state
{
public:
	bean_state()
	{
		variables.clear();
	}

	std::shared_ptr<ast> get_function(const std::string& name)
	{
		for(auto& fun : functions)
		{
			if (fun.first == name)
				return fun.second;
		}

		return nullptr;
	}
	
	std::map<std::string, std::shared_ptr<bean_object>> variables;
	std::map<std::string, std::shared_ptr<ast>> functions;
	//std::vector<std::shared_ptr<bean_function>> functions;
};

class ast
{
public:
	virtual ~ast() = default;
	
	virtual std::shared_ptr<bean_object> eval(bean_state& state)
	{
		throw std::exception("not implemented");
	}

	void set_left(std::shared_ptr<ast> left)
	{
		left_ = left;
	}

	void set_right(std::shared_ptr<ast> right)
	{
		right_ = right;
	}

	std::shared_ptr<ast> get_left()
	{
		return left_;
	}

	std::shared_ptr<ast> get_right()
	{
		return right_;
	}

	void set_value(const token value)
	{
		value_ = value;
	}

	token get_value() const
	{
		return value_;
	}

protected:
	std::shared_ptr<ast> left_;
	std::shared_ptr<ast> right_;
	token value_;
};

class ast_value_double final : public ast
{
	virtual std::shared_ptr<bean_object> eval(bean_state& state) override
	{
		double doubleValue;

		// TODO: Change to a proper str -> number conversion.
		std::stringstream stream;
		stream << value_.get_text();
		stream >> doubleValue;
		return std::make_shared<bean_object_double>(doubleValue);
	}
};

class ast_value_integer final : public ast
{
public:
	virtual std::shared_ptr<bean_object> eval(bean_state& state) override
	{
		std::int32_t intValue;

		// TODO: Change to a proper str -> number conversion.
		std::stringstream stream;
		stream << value_.get_text();
		stream >> intValue;
		return std::make_shared<bean_object_integer>(intValue);
	}
};



class ast_plus final : public ast
{
public:
	virtual std::shared_ptr<bean_object> eval(bean_state& state) override
	{
		return left_->eval(state)->lh_plus(right_->eval(state));
	}
};

class ast_minus final : public ast
{
public:
	virtual std::shared_ptr<bean_object> eval(bean_state& state) override
	{
		return left_->eval(state)->lh_minus(right_->eval(state));
	}
};

class ast_pow final : public ast
{
public:
	virtual std::shared_ptr<bean_object> eval(bean_state& state) override
	{
		return left_->eval(state)->lh_pow(right_->eval(state));
	}
};

class ast_multiply final : public ast
{
public:
	virtual std::shared_ptr<bean_object> eval(bean_state& state) override
	{
		return left_->eval(state)->lh_multiply(right_->eval(state));
	}
};

class ast_divide final : public ast
{
public:
	virtual std::shared_ptr<bean_object> eval(bean_state& state) override
	{
		return left_->eval(state)->lh_divide(right_->eval(state));
	}
};

class ast_define_var final : public ast
{
public:
	virtual std::shared_ptr<bean_object> eval(bean_state& state) override
	{
		const auto varName = left_->get_value().get_text();
		
		state.variables[varName] = std::make_shared<bean_object>(BeanObjectType::None);

		return std::make_shared<bean_object>(BeanObjectType::None);
	}
};

class ast_variable_reference final : public ast
{
public:
	virtual std::shared_ptr<bean_object> eval(bean_state& state) override
	{
		auto variableName = value_.get_text();
		
		return state.variables[variableName];
	}
};

class ast_define_and_set_var final : public ast
{
public:
	virtual std::shared_ptr<bean_object> eval(bean_state& state) override
	{
		auto varName = left_->get_value().get_text();

		state.variables[varName] = right_->eval(state);

		return std::make_shared<bean_object>(BeanObjectType::None);
	}
};

class ast_set_var final : public ast
{
public:
	virtual std::shared_ptr<bean_object> eval(bean_state& state) override
	{
		auto varName = left_->get_value().get_text();
		
		if (state.variables.count(varName) == 0)
			throw std::exception("Invalid variable name!");

		state.variables[varName] = right_->eval(state);
		
		return std::make_shared<bean_object>(BeanObjectType::None);
	}
};

class ast_function final : public ast {
public:
	virtual std::shared_ptr<bean_object> eval(bean_state& state) override
	{
		auto function_name = left_->get_value().get_text();

		state.functions[function_name] = right_;

		return std::make_shared<bean_object>(BeanObjectType::None);
	}
};

class ast_function_script_call final : public ast {
public:
	virtual std::shared_ptr<bean_object> eval(bean_state& state) override
	{
		const auto function_name = left_->get_value().get_text();
		const auto target_function = state.get_function(function_name);

		return target_function->eval(state);
	}
};

class ast_function_call final : public ast
{
public:
	virtual std::shared_ptr<bean_object> eval(bean_state& state) override
	{
		/*
		const auto functionName = left_->get_value().get_text();
		const auto targetFunction = state.get_function(functionName);
		
		if (!targetFunction) {
			throw std::exception("Function to call does not exist!");
		}
		bean_objects params;
		
		if (right_) {
 			params.push_back(right_->eval(state));
		}

		auto function_return = targetFunction->operator()(params);

		if (function_return.size() == 0)
			function_return.push_back(std::make_shared<bean_object>(BeanObjectType::INVALID));

		return function_return[0];
		*/
		return std::make_shared<bean_object>(BeanObjectType::None);
	}
};

inline std::shared_ptr<ast> ast_builder::parse(const token_array& tokens, bean_state& state)
{
	token_iterator iterator(tokens);
	iterator.next();

	handle_parentheses(iterator);

	const auto token_index = iterator.find_rightmost_of_pemdas();
	const auto token = iterator.get_or_invalid(iterator.find_rightmost_of_pemdas());

	if(token_index != invalid_token_index)
	{
		iterator.jump_to(token_index);
	}
	
	if (iterator.get_size() == 1) {
		if (iterator.here().get_type() == token_type::symbol)
		{
			std::shared_ptr<ast> ast_node;

			auto textual_representation = iterator.here().get_text();

			char* p = nullptr;

			auto i = std::strtol(textual_representation.data(), &p, 10);
			if (p == textual_representation.data() + textual_representation.size())
			{
				ast_node = std::make_shared<ast_value_integer>();
			}

			if (!ast_node) {
				auto f = std::strtod(textual_representation.data(), &p);
				if (p == textual_representation.data() + textual_representation.size())
				{
					ast_node = std::make_shared<ast_value_double>();
				}
			}

			if (!ast_node) {
				if (state.variables.count(textual_representation) > 0)
				{
					ast_node = std::make_shared<ast_variable_reference>();
				}
				else
				{
					ast_node = std::make_shared<ast>();
				}
			}

			if (!ast_node)
			{
				throw std::exception("No known type for symbol");
			}

			ast_node->set_value(iterator.here());

			return ast_node;
		}
	}
	else if (token.is_valid()) {

		std::shared_ptr<ast> resulting_node;

		switch (token.get_type())
		{
		case token_type::plus:
			resulting_node = std::make_shared<ast_plus>();
		break;
		case token_type::minus:
			resulting_node = std::make_shared<ast_minus>();
		break;
		case token_type::asterisk:
			resulting_node = std::make_shared<ast_multiply>();
		break;
		case token_type::forward_slash:
			resulting_node = std::make_shared<ast_divide>();
			break;
		case token_type::carrot:
			resulting_node = std::make_shared<ast_pow>();
			break;
		case token_type::equal:
			resulting_node = std::make_shared<ast_set_var>();
		break;
		case token_type::var:
		{
				
			const auto token_name = iterator.splice(token_index + 1, token_index + 2).get_tokens();

			const auto assign_token = iterator.splice(token_index + 2, token_index + 3).get_tokens();

			if(!assign_token.empty() && assign_token[0].get_type() == token_type::equal)
			{
				// We are defining and setting the var.
				resulting_node = std::make_shared<ast_define_and_set_var>();
				const auto var_assign_statement = iterator.splice(token_index + 3, iterator.get_size()).get_tokens();
				resulting_node->set_right(parse(var_assign_statement, state));
			}
			else
			{
				resulting_node = std::make_shared<ast_define_var>();
				resulting_node->set_left(parse(token_name, state));
			}

			resulting_node->set_left(parse(token_name, state));
		}
		case token_type::symbol:
		{

			if(state.get_function(token.get_text()))
			{
				// print function
				resulting_node = std::make_shared<ast_function_script_call>();

				const auto function_name = iterator.splice(token_index + 0, token_index + 1).get_tokens();
				const auto function_args = iterator.splice(token_index + 1, iterator.get_size()).get_tokens();
				
				resulting_node->set_left(parse(function_name, state));

				if(function_args.size() > 2)
					resulting_node->set_right(parse(function_args, state));
			}
			else if(token.get_text() == "fun") {
				// function creation
				/*
				
				fun some_func {
					return some_thing
				}

				*/

				const auto function_name = iterator.splice(token_index + 1, token_index + 2).get_tokens();
				
				iterator.jump_to(token_index + 1);

				const auto function_end = iterator.find_last_pos_of_open_close(token_type::lbrace, token_type::rbrace);

				const auto function_body = iterator.splice(token_index + 3, function_end).get_tokens();

				resulting_node = std::make_shared<ast_function>();
				resulting_node->set_left(parse(function_name, state));
				resulting_node->set_right(parse(function_body, state));

			}
				
		}
		break;
		default:
			resulting_node = std::make_shared<ast>();
			break;
		}

		// Custom operators like var may set these above, don't overwrite them.
		if(!resulting_node->get_left() && !resulting_node->get_right())
		{
			const auto lh_tokens = iterator.splice(0, token_index).get_tokens();
			const auto rh_tokens = iterator.splice(token_index + 1, iterator.get_size()).get_tokens();

			resulting_node->set_left(parse(lh_tokens, state));
			resulting_node->set_right(parse(rh_tokens, state));
		}

		return resulting_node;
	}


	throw std::exception("Reached here!");
}
