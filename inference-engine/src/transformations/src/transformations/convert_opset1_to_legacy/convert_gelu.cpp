// Copyright (C) 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <memory>
#include <ngraph/opsets/opset2.hpp>
#include <transformations/convert_gelu.hpp>
#include <transformations/utils/utils.hpp>

#include <ngraph/ngraph.hpp>

void ngraph::pass::ConvertGELU::convert_gelu() {
    auto input = std::make_shared<pattern::op::Label>(element::f32, Shape{});
    auto gelu = std::make_shared<ngraph::opset2::Gelu>(input);

    ngraph::graph_rewrite_callback callback = [this](pattern::Matcher& m) {
        auto gelu = std::dynamic_pointer_cast<ngraph::opset2::Gelu>(m.get_match_root());
        if (!gelu || transformation_callback(gelu))
            return false;
        auto input = gelu->input_value(0);
        auto input_type = input.get_element_type();

        // f(x) = 0.5 * x * (1.0 + erf( x / sqrt(2.0) )
        auto mul = std::make_shared<ngraph::opset1::Multiply>(input, ngraph::op::Constant::create(input_type, Shape{}, {0.5}));
        auto sq2 = std::make_shared<ngraph::opset1::Sqrt>(ngraph::op::Constant::create(input_type, Shape{}, {2.0}));
        auto div = std::make_shared<ngraph::opset1::Divide>(input, sq2);
        auto erf = std::make_shared<ngraph::opset1::Erf>(div);
        auto add = std::make_shared<ngraph::opset1::Add>(erf, ngraph::op::Constant::create(input_type, Shape{}, {1.0}));
        auto res = std::make_shared<ngraph::opset1::Multiply>(mul, add);

        res->set_friendly_name(gelu->get_friendly_name());
        ngraph::replace_node(gelu, res);
        return true;
    };

    auto m = std::make_shared<ngraph::pattern::Matcher>(gelu, "ConvertGELU");
    this->add_matcher(m, callback, PassProperty::CHANGE_DYNAMIC_STATE);
}
