#pragma once

#define NO_REFLECTION_IMPLEMENTATION_FOUND() \
  static_assert(false, "No reflection implementation found, bailing out!");
