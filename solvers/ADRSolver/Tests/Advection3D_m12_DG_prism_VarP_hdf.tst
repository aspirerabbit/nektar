<?xml version="1.0" encoding="utf-8"?>
<test>
    <description>3D unsteady DG advection, prisms, order 4, P=Variable, HDF output</description>
    <executable>ADRSolver</executable>
    <parameters>--io-format Hdf5 Advection3D_m12_DG_prism_VarP.xml</parameters>
    <files>
        <file description="Session File">Advection3D_m12_DG_prism_VarP.xml</file>
    </files>
    <metrics>
            <metric type="L2" id="1">
            <value variable="u" tolerance="1e-12">1.74811e-05</value>
        </metric>
        <metric type="Linf" id="2">
            <value variable="u" tolerance="1e-12">0.00323074</value>
        </metric>
    </metrics>
</test>

