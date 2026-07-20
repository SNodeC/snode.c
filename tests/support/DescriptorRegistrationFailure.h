/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef TESTS_SUPPORT_DESCRIPTORREGISTRATIONFAILURE_H
#define TESTS_SUPPORT_DESCRIPTORREGISTRATIONFAILURE_H

#include <cstddef>

namespace core::test {

    // Component-test injection only. The next descriptor registration fails
    // after the requested number of successful registrations.
    void failDescriptorRegistrationAfter(std::size_t successfulRegistrations);
    void clearDescriptorRegistrationFailure();

} // namespace core::test

#endif // TESTS_SUPPORT_DESCRIPTORREGISTRATIONFAILURE_H
