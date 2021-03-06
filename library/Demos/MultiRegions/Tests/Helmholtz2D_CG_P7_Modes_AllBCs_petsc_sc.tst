<?xml version="1.0" encoding="utf-8" ?>
<test>
    <description>Helmholtz 2D CG with P=7, all BCs, PETSc sc</description>
    <executable>Helmholtz2D</executable>
    <parameters>-I GlobalSysSoln=PETScStaticCond Helmholtz2D_P7_AllBCs.xml</parameters>
    <files>
        <file description="Session File">Helmholtz2D_P7_AllBCs.xml</file>
    </files>
    <metrics>
        <metric type="L2" id="1">
            <value tolerance="1e-12">0.00888037</value>
        </metric>
        <metric type="Linf" id="2">
            <value tolerance="1e-12">0.0101781</value>
        </metric>
    </metrics>
</test>
