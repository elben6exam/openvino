// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "transformations/op_conversions/convert_maxpool_upgrade.hpp"

#include <ngraph/opsets/opset1.hpp>
#include <ngraph/opsets/opset8.hpp>
#include <ngraph/pattern/op/wrap_type.hpp>
#include <ngraph/rt_info.hpp>
#include <transformations/utils/utils.hpp>

#include "itt.hpp"

ngraph::pass::ConvertMaxPool1ToMaxPool8::ConvertMaxPool1ToMaxPool8() {
    MATCHER_SCOPE(ConvertMaxPool1ToMaxPool8);
    // Replaces v1::MaxPool with v8::MaxPool with default dilations, axis and index_element_type attributes

    auto input = pattern::any_input(pattern::has_static_rank());
    auto maxpool_v1_pattern = ngraph::pattern::wrap_type<ngraph::opset1::MaxPool>({input});

    ngraph::matcher_pass_callback callback = [=](ngraph::pattern::Matcher& m) {
        auto maxpool_v1_node = std::dynamic_pointer_cast<ngraph::opset1::MaxPool>(m.get_match_root());

        if (!maxpool_v1_node)
            return false;

        auto spatial_dims = maxpool_v1_node->get_input_partial_shape(0).rank().get_length() - 2;
        if (spatial_dims <= 0)
            return false;
        ov::Strides dilations(spatial_dims, 1);

        auto maxpool_v8_node = std::make_shared<ngraph::opset8::MaxPool>(maxpool_v1_node->input_value(0),
                                                                         maxpool_v1_node->get_strides(),
                                                                         dilations,
                                                                         maxpool_v1_node->get_pads_begin(),
                                                                         maxpool_v1_node->get_pads_end(),
                                                                         maxpool_v1_node->get_kernel(),
                                                                         maxpool_v1_node->get_rounding_type(),
                                                                         maxpool_v1_node->get_auto_pad(),
                                                                         ov::element::i64,
                                                                         0);

        maxpool_v8_node->set_friendly_name(maxpool_v1_node->get_friendly_name());
        maxpool_v1_node->output(0).replace(maxpool_v8_node->output(0));
        ngraph::copy_runtime_info(maxpool_v1_node, maxpool_v8_node);
        maxpool_v1_node->clear_control_dependencies();
        MATCHER_SCOPE_ENABLE(ConvertMaxPool1ToMaxPool8);
        return true;
    };

    auto m = std::make_shared<pattern::Matcher>(maxpool_v1_pattern, matcher_name);
    register_matcher(m, callback);
}
