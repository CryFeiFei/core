'
' This file is part of the LibreOffice project.
'
' This Source Code Form is subject to the terms of the Mozilla Public
' License, v. 2.0. If a copy of the MPL was not distributed with this
' file, You can obtain one at http://mozilla.org/MPL/2.0/.
'

Function doUnitTest as Integer
    dim aVariant as Object
    ' SWITCH
    If ( Switch( False, 10,_
                 True,  11,_
                 False, 12,_
                 True,  13  ) <> 11 ) Then
        doUnitTest = 0
    Else
        doUnitTest = 1
    End If
End Function
