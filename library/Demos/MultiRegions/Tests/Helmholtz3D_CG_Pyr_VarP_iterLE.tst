<?xml version="1.0" encoding="utf-8" ?>
<test>
    <description>Helmholtz 3D CG for Pyramid with LE preconditioner</description>
    <executable>Helmholtz3D</executable>
    <parameters>-I GlobalSysSoln=IterativeStaticCond -I Preconditioner=LowEnergyBlock Helmholtz3D_Pyr_VarP.xml</parameters>
    <files>
        <file description="Session File">Helmholtz3D_Pyr_VarP.xml</file>
    </files>
    <metrics>
        <metric type="L2" id="1">
            <value tolerance="1e-12">7.99057e-05</value>
        </metric>
        <metric type="Linf" id="2">
            <value tolerance="1e-12">0.000533051</value>
        </metric>
    </metrics>
</test>
