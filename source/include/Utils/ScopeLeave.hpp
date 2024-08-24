#pragma once

#include "Conf/Conf.h"

MAA_NS_BEGIN

template <typename FuncT>
class ScopeLeaveHelper
{
public:
    explicit ScopeLeaveHelper(FuncT func)
        : func_(func)
    {
    }

    ~ScopeLeaveHelper() { func_(); }

private:
    FuncT func_;
};

MAA_NS_END

#define _Cat_(a, b) a##b
#define _Cat(a, b) _Cat_(a, b)
#define _CatVarNameWithLine(Var) _Cat(Var, __LINE__)
#define ScopeHelperVarName _CatVarNameWithLine(_scope_)

#define OnScopeLeave(func) MAA_NS::ScopeLeaveHelper ScopeHelperVarName(func)
