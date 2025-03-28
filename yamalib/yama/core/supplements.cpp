

#include "supplements.h"

#include "scalars.h"
#include "callsig_info.h"
#include "context.h"


using namespace yama::string_literals;


std::vector<yama::type_info> yama::make_supplements() {
    std::vector<type_info> result{};

    auto add_0in1out =
        [&](yama::str name, yama::str return_type, yama::call_fn cf) {
        auto consts =
            const_table_info()
            .add_primitive_type(return_type);
        yama::type_info t{
            .unqualified_name = name,
            .consts = consts,
            .info = function_info{
                .callsig = yama::make_callsig_info({}, 0),
                .call_fn = cf,
                .max_locals = 1,
            },
        };
        result.push_back(t);
        };
    
    auto add_1in0out =
        [&](yama::str name, yama::str param_type, yama::call_fn cf) {
        auto consts =
            const_table_info()
            .add_primitive_type("None"_str)
            .add_primitive_type(param_type);
        yama::type_info t{
            .unqualified_name = name,
            .consts = consts,
            .info = function_info{
                .callsig = yama::make_callsig_info({ 1 }, 0),
                .call_fn = cf,
                .max_locals = 1,
            },
        };
        result.push_back(t);
        };
    
    auto add_1in1out =
        [&](yama::str name, yama::str type, yama::call_fn cf) {
        auto consts =
            const_table_info()
            .add_primitive_type(type);
        yama::type_info t{
            .unqualified_name = name,
            .consts = consts,
            .info = function_info{
                .callsig = yama::make_callsig_info({ 0 }, 0),
                .call_fn = cf,
                .max_locals = 1,
            },
        };
        result.push_back(t);
        };
    
    auto add_2in1out =
        [&](yama::str name, yama::str param_type, yama::str return_type, yama::call_fn cf) {
        auto consts =
            const_table_info()
            .add_primitive_type(param_type)
            .add_primitive_type(return_type);
        yama::type_info t{
            .unqualified_name = name,
            .consts = consts,
            .info = function_info{
                .callsig = yama::make_callsig_info({ 0, 0 }, 1),
                .call_fn = cf,
                .max_locals = 1,
            },
        };
        result.push_back(t);
        };

#define _ADD_0IN1OUT_PROMPT_FN(name, return_type, parse_expr, put_fn) \
add_0in1out( \
    name, return_type, \
    [](context& ctx) { \
        while (true) { \
            std::cerr << "input (" << return_type << "): "; \
            std::string input{}; \
            std::cin >> input; \
            if (const auto result = ( parse_expr )) { \
                if (ctx. put_fn (yama::newtop, result.value().v).bad()) return; \
                break; \
            } \
            std::cerr << "input invalid!\n"; \
        } \
        if (ctx.ret(0).bad()) return; \
    })

#define _ADD_1IN0OUT(name, param_type, expr, as_method) \
add_1in0out( \
    name, param_type, \
    [](context& ctx) { \
        const auto a = ctx.arg(1).value(). as_method (); \
        ( expr ); \
        if (ctx.push_none().bad()) return; \
        if (ctx.ret(0).bad()) return; \
    })
    
#define _ADD_1IN1OUT(name, type_, expr, as_method, put_fn) \
add_1in1out( \
    name, type_, \
    [](context& ctx) { \
        const auto a = ctx.arg(1).value(). as_method (); \
        if (ctx. put_fn (yama::newtop, ( expr )).bad()) return; \
        if (ctx.ret(0).bad()) return; \
    })

#define _ADD_2IN1OUT(name, param_type, return_type, expr, as_method, put_fn) \
add_2in1out( \
    name, param_type, return_type, \
    [](context& ctx) { \
        const auto a = ctx.arg(1).value(). as_method (); \
        const auto b = ctx.arg(2).value(). as_method (); \
        if (ctx. put_fn (yama::newtop, ( expr )).bad()) return; \
        if (ctx.ret(0).bad()) return; \
    })
    
#define _ADD_2IN1OUT_PANIC_IF_B_IS_0(name, param_type, return_type, expr, as_method, put_fn) \
add_2in1out( \
    name, param_type, return_type, \
    [](context& ctx) { \
        const auto a = ctx.arg(1).value(). as_method (); \
        const auto b = ctx.arg(2).value(). as_method (); \
        if (b == (decltype(b))0) { ctx.panic(); return; } \
        if (ctx. put_fn (yama::newtop, ( expr )).bad()) return; \
        if (ctx.ret(0).bad()) return; \
    })

#define _ADD_1IN1OUTS_IUBC(suffix, expr) \
_ADD_1IN1OUT("i" suffix, "Int"_str, expr, as_int, put_int); \
_ADD_1IN1OUT("u" suffix, "UInt"_str, expr, as_uint, put_uint); \
_ADD_1IN1OUT("b" suffix, "Bool"_str, expr, as_bool, put_bool); \
_ADD_1IN1OUT("c" suffix, "Char"_str, expr, as_char, put_char)
    
#define _ADD_1IN1OUTS_IUC(suffix, expr) \
_ADD_1IN1OUT("i" suffix, "Int"_str, expr, as_int, put_int); \
_ADD_1IN1OUT("u" suffix, "UInt"_str, expr, as_uint, put_uint); \
_ADD_1IN1OUT("c" suffix, "Char"_str, expr, as_char, put_char)

#define _ADD_2IN1OUTS_COMMON_R(suffix, return_type, expr, put_fn) \
_ADD_2IN1OUT("i" suffix, "Int"_str, return_type, expr, as_int, put_fn); \
_ADD_2IN1OUT("u" suffix, "UInt"_str, return_type, expr, as_uint, put_fn); \
_ADD_2IN1OUT("f" suffix, "Float"_str, return_type, expr, as_float, put_fn); \
_ADD_2IN1OUT("b" suffix, "Bool"_str, return_type, expr, as_bool, put_fn); \
_ADD_2IN1OUT("c" suffix, "Char"_str, return_type, expr, as_char, put_fn)
    
#define _ADD_2IN1OUTS(suffix, expr) \
_ADD_2IN1OUT("i" suffix, "Int"_str, "Int"_str, expr, as_int, put_int); \
_ADD_2IN1OUT("u" suffix, "UInt"_str, "UInt"_str, expr, as_uint, put_uint); \
_ADD_2IN1OUT("f" suffix, "Float"_str, "Float"_str, expr, as_float, put_float); \
_ADD_2IN1OUT("b" suffix, "Bool"_str, "Bool"_str, expr, as_bool, put_bool); \
_ADD_2IN1OUT("c" suffix, "Char"_str, "Char"_str, expr, as_char, put_char)
    
#define _ADD_2IN1OUTS_IUBC(suffix, expr) \
_ADD_2IN1OUT("i" suffix, "Int"_str, "Int"_str, expr, as_int, put_int); \
_ADD_2IN1OUT("u" suffix, "UInt"_str, "UInt"_str, expr, as_uint, put_uint); \
_ADD_2IN1OUT("b" suffix, "Bool"_str, "Bool"_str, expr, as_bool, put_bool); \
_ADD_2IN1OUT("c" suffix, "Char"_str, "Char"_str, expr, as_char, put_char)
    
#define _ADD_2IN1OUTS_IUF(suffix, expr) \
_ADD_2IN1OUT("i" suffix, "Int"_str, "Int"_str, expr, as_int, put_int); \
_ADD_2IN1OUT("u" suffix, "UInt"_str, "UInt"_str, expr, as_uint, put_uint); \
_ADD_2IN1OUT("f" suffix, "Float"_str, "Float"_str, expr, as_float, put_float)
    
#define _ADD_2IN1OUTS_IUF_PANIC_IF_B_IS_0(suffix, expr) \
_ADD_2IN1OUT_PANIC_IF_B_IS_0("i" suffix, "Int"_str, "Int"_str, expr, as_int, put_int); \
_ADD_2IN1OUT_PANIC_IF_B_IS_0("u" suffix, "UInt"_str, "UInt"_str, expr, as_uint, put_uint); \
_ADD_2IN1OUT_PANIC_IF_B_IS_0("f" suffix, "Float"_str, "Float"_str, expr, as_float, put_float)

#define _ADD_2IN1OUTS_IU_PANIC_IF_B_IS_0(suffix, expr) \
_ADD_2IN1OUT_PANIC_IF_B_IS_0("i" suffix, "Int"_str, "Int"_str, expr, as_int, put_int); \
_ADD_2IN1OUT_PANIC_IF_B_IS_0("u" suffix, "UInt"_str, "UInt"_str, expr, as_uint, put_uint)

    // comparison ops
    _ADD_2IN1OUTS_COMMON_R("eq"_str, "Bool"_str, a == b, put_bool);
    _ADD_2IN1OUTS_COMMON_R("ne"_str, "Bool"_str, a != b, put_bool);
    _ADD_2IN1OUTS_COMMON_R("gt"_str, "Bool"_str, a > b, put_bool);
    _ADD_2IN1OUTS_COMMON_R("lt"_str, "Bool"_str, a < b, put_bool);
    _ADD_2IN1OUTS_COMMON_R("ge"_str, "Bool"_str, a >= b, put_bool);
    _ADD_2IN1OUTS_COMMON_R("le"_str, "Bool"_str, a <= b, put_bool);

    // arithmetic ops
    _ADD_2IN1OUTS_IUF("add"_str, a + b);
    _ADD_2IN1OUTS_IUF("sub"_str, a - b);
    _ADD_2IN1OUTS_IUF("mul"_str, a * b);
    _ADD_2IN1OUTS_IUF_PANIC_IF_B_IS_0("div"_str, a / b);
    _ADD_2IN1OUTS_IU_PANIC_IF_B_IS_0("mod"_str, a % b);
    _ADD_1IN1OUT("ineg"_str, "Int"_str, -a, as_int, put_int);
    _ADD_1IN1OUT("fneg"_str, "Float"_str, -a, as_float, put_float);

    // logical ops
    _ADD_2IN1OUT("band"_str, "Bool"_str, "Bool"_str, a && b, as_bool, put_bool);
    _ADD_2IN1OUT("bor"_str, "Bool"_str, "Bool"_str, a || b, as_bool, put_bool);
    _ADD_2IN1OUT("bxor"_str, "Bool"_str, "Bool"_str, a ^ b, as_bool, put_bool);
    _ADD_1IN1OUT("bnot"_str, "Bool"_str, !a, as_bool, put_bool);

    // bitwise ops
    _ADD_2IN1OUTS_IUBC("bit_and"_str, a & b);
    _ADD_2IN1OUTS_IUBC("bit_or"_str, a | b);
    _ADD_2IN1OUTS_IUBC("bit_xor"_str, a ^ b);
    _ADD_1IN1OUTS_IUC("bit_not"_str, ~a);
    // we'll just manually write the bit-shift ops
    {
        auto consts =
            const_table_info()
            .add_primitive_type("Int"_str)
            .add_primitive_type("UInt"_str);
        auto cf =
            [](context& ctx) {
            const auto a = ctx.arg(1).value().as_int();
            const auto b = ctx.arg(2).value().as_uint();
            if (ctx.push_int(a << b).bad()) return;
            if (ctx.ret(0).bad()) return;
            };
        yama::type_info t{
            .unqualified_name = "ibit_lshift"_str,
            .consts = consts,
            .info = function_info{
                .callsig = yama::make_callsig_info({ 0, 1 }, 0),
                .call_fn = cf,
                .max_locals = 1,
            },
        };
        result.push_back(t);
    }
    {
        auto consts =
            const_table_info()
            .add_primitive_type("Int"_str)
            .add_primitive_type("UInt"_str);
        auto cf =
            [](context& ctx) {
            const auto a = ctx.arg(1).value().as_int();
            const auto b = ctx.arg(2).value().as_uint();
            if (ctx.push_int(a >> b).bad()) return;
            if (ctx.ret(0).bad()) return;
            };
        yama::type_info t{
            .unqualified_name = "ibit_rshift"_str,
            .consts = consts,
            .info = function_info{
                .callsig = yama::make_callsig_info({ 0, 1 }, 0),
                .call_fn = cf,
                .max_locals = 1,
            },
        };
        result.push_back(t);
    }
    {
        auto consts =
            const_table_info()
            .add_primitive_type("UInt"_str);
        auto cf =
            [](context& ctx) {
            const auto a = ctx.arg(1).value().as_uint();
            const auto b = ctx.arg(2).value().as_uint();
            if (ctx.push_int(a << b).bad()) return;
            if (ctx.ret(0).bad()) return;
            };
        yama::type_info t{
            .unqualified_name = "ubit_lshift"_str,
            .consts = consts,
            .info = function_info{
                .callsig = yama::make_callsig_info({ 0, 0 }, 0),
                .call_fn = cf,
                .max_locals = 1,
            },
        };
        result.push_back(t);
    }
    {
        auto consts =
            const_table_info()
            .add_primitive_type("UInt"_str);
        auto cf =
            [](context& ctx) {
            const auto a = ctx.arg(1).value().as_uint();
            const auto b = ctx.arg(2).value().as_uint();
            if (ctx.push_int(a >> b).bad()) return;
            if (ctx.ret(0).bad()) return;
            };
        yama::type_info t{
            .unqualified_name = "ubit_rshift"_str,
            .consts = consts,
            .info = function_info{
                .callsig = yama::make_callsig_info({ 0, 0 }, 0),
                .call_fn = cf,
                .max_locals = 1,
            },
        };
        result.push_back(t);
    }

    // print fns
    _ADD_1IN0OUT("iprint"_str, "Int"_str, (std::cerr << " > " << yama::fmt_int(a) << "\n"), as_int);
    _ADD_1IN0OUT("uprint"_str, "UInt"_str, (std::cerr << " > " << yama::fmt_uint(a) << "\n"), as_uint);
    _ADD_1IN0OUT("fprint"_str, "Float"_str, (std::cerr << " > " << yama::fmt_float(a) << "\n"), as_float);
    _ADD_1IN0OUT("bprint"_str, "Bool"_str, (std::cerr << " > " << yama::fmt_bool(a) << "\n"), as_bool);
    _ADD_1IN0OUT("cprint"_str, "Char"_str, (std::cerr << " > " << yama::fmt_char(a) << "\n"), as_char);

    // prompt fns (which await valid user input)
    _ADD_0IN1OUT_PROMPT_FN("iprompt"_str, "Int"_str, yama::parse_int(input), put_int);
    _ADD_0IN1OUT_PROMPT_FN("uprompt"_str, "UInt"_str, yama::parse_uint(input), put_uint);
    _ADD_0IN1OUT_PROMPT_FN("fprompt"_str, "Float"_str, yama::parse_float(input), put_float);
    _ADD_0IN1OUT_PROMPT_FN("bprompt"_str, "Bool"_str, yama::parse_bool(input), put_bool);
    _ADD_0IN1OUT_PROMPT_FN("cprompt"_str, "Char"_str, yama::parse_char(input), put_char);

    // panic fn
    {
        auto consts =
            const_table_info()
            .add_primitive_type("None"_str);
        yama::type_info t{
            .unqualified_name = "panic"_str,
            .consts = consts,
            .info = function_info{
                .callsig = yama::make_callsig_info({}, 0),
                .call_fn = [](context& ctx) { ctx.panic(); },
                .max_locals = 1,
            },
        };
        result.push_back(t);
    }

#define _ADD_CONV_FN(in_letter, out_letter, in_type, out_type, out_cpp_type, in_as_method, out_put_fn) \
{ \
    auto consts = \
        const_table_info() \
        .add_primitive_type(in_type) \
        .add_primitive_type(out_type); \
    auto cf = \
        [](context& ctx) { \
            const auto a = ctx.arg(1).value(). in_as_method (); \
            if (ctx. out_put_fn (yama::newtop, ( out_cpp_type )a).bad()) return; \
            if (ctx.ret(0).bad()) return; \
        }; \
    yama::type_info t{ \
        .unqualified_name = in_letter "2" out_letter ""_str, \
        .consts = consts, \
        .info = function_info{ \
            .callsig = yama::make_callsig_info({ 0 }, 1), \
            .call_fn = cf, \
            .max_locals = 1, \
        }, \
    }; \
    result.push_back(t); \
} (void)0

#define _ADD_CONV_FNS(in_letter, in_type, in_as_method) \
_ADD_CONV_FN(in_letter, "i", in_type, "Int"_str, yama::int_t, in_as_method, put_int); \
_ADD_CONV_FN(in_letter, "u", in_type, "UInt"_str, yama::uint_t, in_as_method, put_uint); \
_ADD_CONV_FN(in_letter, "f", in_type, "Float"_str, yama::float_t, in_as_method, put_float); \
_ADD_CONV_FN(in_letter, "b", in_type, "Bool"_str, yama::bool_t, in_as_method, put_bool); \
_ADD_CONV_FN(in_letter, "c", in_type, "Char"_str, yama::char_t, in_as_method, put_char)

    // conversion fns
    _ADD_CONV_FNS("i", "Int"_str, as_int);
    _ADD_CONV_FNS("u", "UInt"_str, as_uint);
    _ADD_CONV_FNS("f", "Float"_str, as_float);
    _ADD_CONV_FNS("b", "Bool"_str, as_bool);
    _ADD_CONV_FNS("c", "Char"_str, as_char);

    return result;
}

