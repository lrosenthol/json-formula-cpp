namespace jsonformula {

    enum class operator_kind
    {
        default_op, // Identifier, CurrentNode, Index, MultiSelectList, MultiSelectHash, FunctionExpression
        projection_op,
        flatten_projection_op, // FlattenProjection
        or_op,
        and_op,
        eq_op,
        ne_op,
        lt_op,
        lte_op,
        gt_op,
        gte_op,
        not_op,
		// json-formula
		plus_op,
		minus_op,
		mult_op,
		div_op,
		concat_op,
		union_op
    };

    struct operator_table final
    {
        static int precedence_level(operator_kind oper)
        {
            switch (oper)
            {
                case operator_kind::projection_op:
                    return 11;
                case operator_kind::flatten_projection_op:
                    return 11;
                case operator_kind::or_op:
				case operator_kind::plus_op:	// json-formula. right place?
				case operator_kind::minus_op:
				case operator_kind::mult_op:
				case operator_kind::div_op:
				case operator_kind::concat_op:
				case operator_kind::union_op:
                    return 9;
                case operator_kind::and_op:
                    return 8;
                case operator_kind::eq_op:
                case operator_kind::ne_op:
                    return 6;
                case operator_kind::lt_op:
                case operator_kind::lte_op:
                case operator_kind::gt_op:
                case operator_kind::gte_op:
                    return 5;
                case operator_kind::not_op:
                    return 1;
                default:
                    return 1;
            }
        }

        static bool is_right_associative(operator_kind oper)
        {
            switch (oper)
            {
                case operator_kind::not_op:
                    return true;
                case operator_kind::projection_op:
                    return true;
                case operator_kind::flatten_projection_op:
                    return false;
                case operator_kind::or_op:
                case operator_kind::and_op:
                case operator_kind::eq_op:
                case operator_kind::ne_op:
                case operator_kind::lt_op:
                case operator_kind::lte_op:
                case operator_kind::gt_op:
                case operator_kind::gte_op:
                    return false;
                default:
                    return false;
            }
        }
    };

    enum class token_kind 
    {
        current_node,
        lparen,
        rparen,
        begin_multi_select_hash,
        end_multi_select_hash,
        begin_multi_select_list,
        end_multi_select_list,
        begin_filter,
        end_filter,
        pipe,
        separator,
        key,
        literal,
        expression,
        binary_operator,
        unary_operator,
        function,
        end_function,
        argument,
        begin_expression_type,
        end_expression_type,
        end_of_expression
    };

    struct literal_arg_t
    {
        explicit literal_arg_t() = default;
    };
    constexpr literal_arg_t literal_arg{};

    struct begin_expression_type_arg_t
    {
        explicit begin_expression_type_arg_t() = default;
    };
    constexpr begin_expression_type_arg_t begin_expression_type_arg{};

    struct end_expression_type_arg_t
    {
        explicit end_expression_type_arg_t() = default;
    };
    constexpr end_expression_type_arg_t end_expression_type_arg{};

    struct end_of_expression_arg_t
    {
        explicit end_of_expression_arg_t() = default;
    };
    constexpr end_of_expression_arg_t end_of_expression_arg{};

    struct separator_arg_t
    {
        explicit separator_arg_t() = default;
    };
    constexpr separator_arg_t separator_arg{};

    struct key_arg_t
    {
        explicit key_arg_t() = default;
    };
    constexpr key_arg_t key_arg{};

    struct lparen_arg_t
    {
        explicit lparen_arg_t() = default;
    };
    constexpr lparen_arg_t lparen_arg{};

    struct rparen_arg_t
    {
        explicit rparen_arg_t() = default;
    };
    constexpr rparen_arg_t rparen_arg{};

    struct begin_multi_select_hash_arg_t
    {
        explicit begin_multi_select_hash_arg_t() = default;
    };
    constexpr begin_multi_select_hash_arg_t begin_multi_select_hash_arg{};

    struct end_multi_select_hash_arg_t
    {
        explicit end_multi_select_hash_arg_t() = default;
    };
    constexpr end_multi_select_hash_arg_t end_multi_select_hash_arg{};

    struct begin_multi_select_list_arg_t
    {
        explicit begin_multi_select_list_arg_t() = default;
    };
    constexpr begin_multi_select_list_arg_t begin_multi_select_list_arg{};

    struct end_multi_select_list_arg_t
    {
        explicit end_multi_select_list_arg_t() = default;
    };
    constexpr end_multi_select_list_arg_t end_multi_select_list_arg{};

    struct begin_filter_arg_t
    {
        explicit begin_filter_arg_t() = default;
    };
    constexpr begin_filter_arg_t begin_filter_arg{};

    struct end_filter_arg_t
    {
        explicit end_filter_arg_t() = default;
    };
    constexpr end_filter_arg_t end_filter_arg{};

    struct pipe_arg_t
    {
        explicit pipe_arg_t() = default;
    };
    constexpr pipe_arg_t pipe_arg{};

    struct current_node_arg_t
    {
        explicit current_node_arg_t() = default;
    };
    constexpr current_node_arg_t current_node_arg{};

    struct end_function_arg_t
    {
        explicit end_function_arg_t() = default;
    };
    constexpr end_function_arg_t end_function_arg{};

    struct argument_arg_t
    {
        explicit argument_arg_t() = default;
    };
    constexpr argument_arg_t argument_arg{};

    struct slice
    {
        jsoncons::optional<int64_t> start_;
        jsoncons::optional<int64_t> stop_;
        int64_t step_;

        slice()
            : start_(), stop_(), step_(1)
        {
        }

        slice(const jsoncons::optional<int64_t>& start, const jsoncons::optional<int64_t>& end, int64_t step) 
            : start_(start), stop_(end), step_(step)
        {
        }

        slice(const slice& other)
            : start_(other.start_), stop_(other.stop_), step_(other.step_)
        {
        }

        slice& operator=(const slice& rhs) 
        {
            if (this != &rhs)
            {
                if (rhs.start_)
                {
                    start_ = rhs.start_;
                }
                else
                {
                    start_.reset();
                }
                if (rhs.stop_)
                {
                    stop_ = rhs.stop_;
                }
                else
                {
                    stop_.reset();
                }
                step_ = rhs.step_;
            }
            return *this;
        }

        int64_t get_start(std::size_t size) const
        {
            if (start_)
            {
                auto len = *start_ >= 0 ? *start_ : (static_cast<int64_t>(size) + *start_);
                return len <= static_cast<int64_t>(size) ? len : static_cast<int64_t>(size);
            }
            else
            {
                if (step_ >= 0)
                {
                    return 0;
                }
                else 
                {
                    return static_cast<int64_t>(size);
                }
            }
        }

        int64_t get_stop(std::size_t size) const
        {
            if (stop_)
            {
                auto len = *stop_ >= 0 ? *stop_ : (static_cast<int64_t>(size) + *stop_);
                return len <= static_cast<int64_t>(size) ? len : static_cast<int64_t>(size);
            }
            else
            {
                return step_ >= 0 ? static_cast<int64_t>(size) : -1;
            }
        }

        int64_t step() const
        {
            return step_; // Allow negative
        }
    };


}
