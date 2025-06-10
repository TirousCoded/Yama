

#include <gtest/gtest.h>

#include <yama/core/const_table_info.h>
#include <yama/core/const_table.h>
#include <yama/core/type_info.h>


using namespace yama::string_literals;


// helper data

const yama::str old_name = "abc"_str;
const yama::str new_name = "def"_str;

const yama::const_table_info old_consts =
yama::const_table_info()
.add_bool(true)
.add_bool(false)
.add_int(50'000);
const yama::const_table_info new_consts =
yama::const_table_info()
.add_float(3.14159)
.add_float(3.14159)
.add_float(3.14159);

const yama::ptype old_ptype = yama::ptype::bool0;
const yama::ptype new_ptype = yama::ptype::uint;

void old_cf(yama::context&) {};
void new_cf(yama::context&) {};

const yama::callsig_info old_callsig = yama::make_callsig({ 0, 1, 2 }, 1);
const yama::callsig_info new_callsig = yama::make_callsig({ 2, 2 }, 2);

const size_t old_max_locals = 17;
const size_t new_max_locals = 50'000;

const yama::bc::code old_bcode =
yama::bc::code_writer()
.add_noop()
.add_noop()
.add_noop()
.done().value();
const yama::bc::code new_bcode =
yama::bc::code_writer()
.add_noop()
.add_noop()
.add_noop()
.add_noop()
.add_noop()
.add_noop()
.done().value();

const yama::bc::syms old_bsyms =
yama::bc::syms()
.add(10, "abcdef"_str, 50'000, 13'010);
const yama::bc::syms new_bsyms =
yama::bc::syms()
.add(101, "abcdefghi"_str, 150'000, 113'010)
.add(10, "abcdef"_str, 50'000, 13'010);


// IMPORTANT: update this as we add new properties

void change(yama::type_info& x) {
    x.change_unqualified_name(new_name);
    x.change_consts(new_consts);
    x.change_ptype(new_ptype);
    x.change_callsig(new_callsig);
    x.change_call_fn(new_cf);
    x.change_max_locals(new_max_locals);
    x.change_bcode(new_bcode);
    x.change_bsyms(new_bsyms);
}


static_assert(yama::kinds == 3); // reminder

TEST(TypeInfoTests, Primitive) {
    auto abc = yama::make_primitive(old_name, old_consts, old_ptype);

    EXPECT_EQ(abc.unqualified_name(), old_name);
    EXPECT_EQ(abc.consts(), old_consts);
    EXPECT_EQ(abc.kind(), yama::kind::primitive);
    EXPECT_EQ(abc.ptype(), old_ptype);
    EXPECT_EQ(abc.callsig(), nullptr);
    EXPECT_EQ(abc.max_locals(), 0);
    EXPECT_EQ(abc.call_fn(), std::nullopt);
    EXPECT_EQ(abc.bcode(), nullptr);
    EXPECT_EQ(abc.bsyms(), nullptr);
    EXPECT_EQ(abc.uses_bcode(), false);

    change(abc);

    EXPECT_EQ(abc.unqualified_name(), new_name);
    EXPECT_EQ(abc.consts(), new_consts);
    EXPECT_EQ(abc.kind(), yama::kind::primitive);
    EXPECT_EQ(abc.ptype(), new_ptype);
    EXPECT_EQ(abc.callsig(), nullptr);
    EXPECT_EQ(abc.max_locals(), 0);
    EXPECT_EQ(abc.call_fn(), std::nullopt);
    EXPECT_EQ(abc.bcode(), nullptr);
    EXPECT_EQ(abc.bsyms(), nullptr);
    EXPECT_EQ(abc.uses_bcode(), false);
}

TEST(TypeInfoTests, Function_NonBCode) {
    auto abc = yama::make_function(old_name, old_consts, old_callsig, old_max_locals, old_cf);

    EXPECT_EQ(abc.unqualified_name(), old_name);
    EXPECT_EQ(abc.consts(), old_consts);
    EXPECT_EQ(abc.kind(), yama::kind::function);
    EXPECT_EQ(abc.ptype(), std::nullopt);
    if (abc.callsig()) EXPECT_EQ(*abc.callsig(), old_callsig); else ADD_FAILURE();
    EXPECT_EQ(abc.max_locals(), old_max_locals);
    EXPECT_EQ(abc.call_fn(), std::make_optional(old_cf));
    if (abc.bcode()) EXPECT_EQ(*abc.bcode(), yama::bc::code{}); else ADD_FAILURE();
    if (abc.bsyms()) EXPECT_EQ(*abc.bsyms(), yama::bc::syms{}); else ADD_FAILURE();
    EXPECT_EQ(abc.uses_bcode(), false);

    change(abc);

    EXPECT_EQ(abc.unqualified_name(), new_name);
    EXPECT_EQ(abc.consts(), new_consts);
    EXPECT_EQ(abc.kind(), yama::kind::function);
    EXPECT_EQ(abc.ptype(), std::nullopt);
    if (abc.callsig()) EXPECT_EQ(*abc.callsig(), new_callsig); else ADD_FAILURE();
    EXPECT_EQ(abc.max_locals(), new_max_locals);
    EXPECT_EQ(abc.call_fn(), std::make_optional(new_cf));
    if (abc.bcode()) EXPECT_EQ(*abc.bcode(), new_bcode); else ADD_FAILURE();
    if (abc.bsyms()) EXPECT_EQ(*abc.bsyms(), new_bsyms); else ADD_FAILURE();
    EXPECT_EQ(abc.uses_bcode(), false);
}

TEST(TypeInfoTests, Function_BCode) {
    auto abc = yama::make_function(old_name, old_consts, old_callsig, old_max_locals, old_bcode, old_bsyms);

    EXPECT_EQ(abc.unqualified_name(), old_name);
    EXPECT_EQ(abc.consts(), old_consts);
    EXPECT_EQ(abc.kind(), yama::kind::function);
    EXPECT_EQ(abc.ptype(), std::nullopt);
    if (abc.callsig()) EXPECT_EQ(*abc.callsig(), old_callsig); else ADD_FAILURE();
    EXPECT_EQ(abc.max_locals(), old_max_locals);
    EXPECT_EQ(abc.call_fn(), std::make_optional(yama::bcode_call_fn));
    if (abc.bcode()) EXPECT_EQ(*abc.bcode(), old_bcode); else ADD_FAILURE();
    if (abc.bsyms()) EXPECT_EQ(*abc.bsyms(), old_bsyms); else ADD_FAILURE();
    EXPECT_EQ(abc.uses_bcode(), true);

    change(abc);

    EXPECT_EQ(abc.unqualified_name(), new_name);
    EXPECT_EQ(abc.consts(), new_consts);
    EXPECT_EQ(abc.kind(), yama::kind::function);
    EXPECT_EQ(abc.ptype(), std::nullopt);
    if (abc.callsig()) EXPECT_EQ(*abc.callsig(), new_callsig); else ADD_FAILURE();
    EXPECT_EQ(abc.max_locals(), new_max_locals);
    EXPECT_EQ(abc.call_fn(), std::make_optional(new_cf));
    if (abc.bcode()) EXPECT_EQ(*abc.bcode(), new_bcode); else ADD_FAILURE();
    if (abc.bsyms()) EXPECT_EQ(*abc.bsyms(), new_bsyms); else ADD_FAILURE();
    EXPECT_EQ(abc.uses_bcode(), false);
}

TEST(TypeInfoTests, Struct) {
    auto abc = yama::make_struct(old_name, old_consts);

    EXPECT_EQ(abc.unqualified_name(), old_name);
    EXPECT_EQ(abc.consts(), old_consts);
    EXPECT_EQ(abc.kind(), yama::kind::struct0);
    EXPECT_EQ(abc.ptype(), std::nullopt);
    EXPECT_EQ(abc.callsig(), nullptr);
    EXPECT_EQ(abc.max_locals(), 0);
    EXPECT_EQ(abc.call_fn(), std::nullopt);
    EXPECT_EQ(abc.bcode(), nullptr);
    EXPECT_EQ(abc.bsyms(), nullptr);
    EXPECT_EQ(abc.uses_bcode(), false);

    change(abc);

    EXPECT_EQ(abc.unqualified_name(), new_name);
    EXPECT_EQ(abc.consts(), new_consts);
    EXPECT_EQ(abc.kind(), yama::kind::struct0);
    EXPECT_EQ(abc.ptype(), std::nullopt);
    EXPECT_EQ(abc.callsig(), nullptr);
    EXPECT_EQ(abc.max_locals(), 0);
    EXPECT_EQ(abc.call_fn(), std::nullopt);
    EXPECT_EQ(abc.bcode(), nullptr);
    EXPECT_EQ(abc.bsyms(), nullptr);
    EXPECT_EQ(abc.uses_bcode(), false);
}


// these unit tests are for ensuring that impl deep-copies

TEST(TypeInfoTests, CopyCtor) {
    auto original = yama::make_function(old_name, old_consts, old_callsig, old_max_locals, old_bcode, old_bsyms);

    yama::type_info copy(original); // copy ctor

    // changing original shouldn't affect copy

    change(original);

    EXPECT_EQ(copy.unqualified_name(), old_name);
    EXPECT_EQ(copy.consts(), old_consts);
    EXPECT_EQ(copy.kind(), yama::kind::function);
    EXPECT_EQ(copy.ptype(), std::nullopt);
    if (copy.callsig()) EXPECT_EQ(*copy.callsig(), old_callsig); else ADD_FAILURE();
    EXPECT_EQ(copy.max_locals(), old_max_locals);
    EXPECT_EQ(copy.call_fn(), std::make_optional(yama::bcode_call_fn));
    if (copy.bcode()) EXPECT_EQ(*copy.bcode(), old_bcode); else ADD_FAILURE();
    if (copy.bsyms()) EXPECT_EQ(*copy.bsyms(), old_bsyms); else ADD_FAILURE();
    EXPECT_EQ(copy.uses_bcode(), true);
}

TEST(TypeInfoTests, CopyAssign) {
    auto original = yama::make_function(old_name, old_consts, old_callsig, old_max_locals, old_bcode, old_bsyms);
    auto copy = yama::make_primitive(""_str, {}, yama::ptype::bool0);

    copy = original; // copy assign

    // changing original shouldn't affect copy

    change(original);

    EXPECT_EQ(copy.unqualified_name(), old_name);
    EXPECT_EQ(copy.consts(), old_consts);
    EXPECT_EQ(copy.kind(), yama::kind::function);
    EXPECT_EQ(copy.ptype(), std::nullopt);
    if (copy.callsig()) EXPECT_EQ(*copy.callsig(), old_callsig); else ADD_FAILURE();
    EXPECT_EQ(copy.max_locals(), old_max_locals);
    EXPECT_EQ(copy.call_fn(), std::make_optional(yama::bcode_call_fn));
    if (copy.bcode()) EXPECT_EQ(*copy.bcode(), old_bcode); else ADD_FAILURE();
    if (copy.bsyms()) EXPECT_EQ(*copy.bsyms(), old_bsyms); else ADD_FAILURE();
    EXPECT_EQ(copy.uses_bcode(), true);
}

