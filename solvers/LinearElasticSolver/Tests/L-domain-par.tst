<?xml version="1.0" encoding="utf-8" ?>
<test>
    <description>Linear elastic solver P=4</description>
    <executable>LinearElasticSolver</executable>
    <parameters>L-domain.xml</parameters>
    <processes>2</processes>
    <files>
        <file description="Session File">L-domain.xml</file>
    </files>
    <metrics>
        <metric type="L2" id="1">
            <value variable="u" tolerance="1e-12">0.0564306</value>
            <value variable="v" tolerance="1e-12">0.0212315</value>
        </metric>
        <metric type="Linf" id="2">
            <value variable="u" tolerance="1e-12">0.0565293</value>
            <value variable="v" tolerance="1e-12">0.0695632</value>
        </metric>
    </metrics>
</test>
