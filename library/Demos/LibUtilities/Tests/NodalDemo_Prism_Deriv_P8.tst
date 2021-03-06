<?xml version="1.0" encoding="utf-8" ?>
<test>
    <description>Nodal prism derivative, evenly spaced points, P = 8</description>
    <executable>NodalDemo</executable>
    <parameters>--order 8 --type 27 --deriv</parameters>
    <metrics>
        <metric type="L2" id="1">
            <value variable="u" tolerance="1e-12">0.000197703</value>
            <value variable="v" tolerance="1e-9">5.30087e-07</value>
            <value variable="w" tolerance="1e-12">0.000197703</value>
        </metric>
    </metrics>
</test>
