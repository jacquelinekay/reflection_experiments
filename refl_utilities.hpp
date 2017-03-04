#pragma once


template <typename... Members>
using member_pack_as_tuple = std::tuple<
    std::meta::get_reflected_type_t<std::meta::get_type_m<Members>>...>;

