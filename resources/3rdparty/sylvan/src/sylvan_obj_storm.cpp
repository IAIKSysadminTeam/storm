Mtbdd
Bdd::toDoubleMtbdd() const {
    LACE_ME;
    return mtbdd_bool_to_double(bdd);
}

Mtbdd
Mtbdd::Minus(const Mtbdd &other) const
{
    LACE_ME;
    return mtbdd_minus(mtbdd, other.mtbdd);
}

Mtbdd
Mtbdd::Divide(const Mtbdd &other) const
{
    LACE_ME;
    return mtbdd_divide(mtbdd, other.mtbdd);
}

Bdd
Mtbdd::NotZero() const
{
    LACE_ME;
    return mtbdd_not_zero(mtbdd);
}

Bdd
Mtbdd::Equals(const Mtbdd& other) const {
    LACE_ME;
    return mtbdd_equals(mtbdd, other.mtbdd);
}

Bdd
Mtbdd::Less(const Mtbdd& other) const {
    LACE_ME;
    return mtbdd_less_as_bdd(mtbdd, other.mtbdd);
}

Bdd
Mtbdd::LessOrEqual(const Mtbdd& other) const {
    LACE_ME;
    return mtbdd_less_or_equal_as_bdd(mtbdd, other.mtbdd);
}

double
Mtbdd::getDoubleMax() const {
    LACE_ME;
    MTBDD maxNode = mtbdd_maximum(mtbdd);
    return mtbdd_getdouble(maxNode);
}

double
Mtbdd::getDoubleMin() const {
    LACE_ME;
    MTBDD minNode = mtbdd_minimum(mtbdd);
    return mtbdd_getdouble(minNode);
}

bool
Mtbdd::EqualNorm(const Mtbdd& other, double epsilon) const {
    LACE_ME;
    return mtbdd_equal_norm_d(mtbdd, other.mtbdd, epsilon);
}

bool
Mtbdd::EqualNormRel(const Mtbdd& other, double epsilon) const {
    LACE_ME;
    return mtbdd_equal_norm_rel_d(mtbdd, other.mtbdd, epsilon);
}

Mtbdd
Mtbdd::Floor() const {
    LACE_ME;
    return mtbdd_floor(mtbdd);
}

Mtbdd
Mtbdd::Ceil() const {
    LACE_ME;
    return mtbdd_ceil(mtbdd);
}

Mtbdd
Mtbdd::Pow(const Mtbdd& other) const {
    LACE_ME;
    return mtbdd_pow(mtbdd, other.mtbdd);
}

Mtbdd
Mtbdd::Mod(const Mtbdd& other) const {
    LACE_ME;
    return mtbdd_mod(mtbdd, other.mtbdd);
}

Mtbdd
Mtbdd::Logxy(const Mtbdd& other) const {
    LACE_ME;
    return mtbdd_logxy(mtbdd, other.mtbdd);
}

size_t
Mtbdd::CountLeaves() const {
    LACE_ME;
    return mtbdd_leafcount(mtbdd);
}

double
Mtbdd::NonZeroCount(size_t variableCount) const {
    LACE_ME;
    return mtbdd_non_zero_count(mtbdd, variableCount);
}

bool
Mtbdd::isValid() const {
    LACE_ME;
    return mtbdd_test_isvalid(mtbdd) == 1;
}