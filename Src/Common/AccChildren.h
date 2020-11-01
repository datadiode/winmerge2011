// AccChildren.h
// Copyright (c) datadiode
// SPDX-License-Identifier: WTFPL

class AccChildren
{
public:
    enum FindFlags
    {
        RoleMask = 0x0FFF,
        ByAccelerator = 0x1000 /**< searches by accelerator rather than name */
    };
    AccChildren(IAccessible *accessible)
        : m_count(count(accessible)), m_sa(sa()), m_data(data())
    {
        long count = m_count;
        AccessibleChildren(accessible, 0, m_count, m_data, &count);
    }
    ~AccChildren()
    {
        SafeArrayUnaccessData(m_sa);
        SafeArrayDestroy(m_sa);
    }
    operator void const *() const { return m_data; }
    IAccessible *Find(LPCWSTR name, long kind) const
    {
        VARIANT varChild;
        varChild.vt = VT_I4;
        varChild.lVal = 0;
        for (long index = 0; index < m_count; ++index)
        {
            IAccessible *const child = static_cast
                <IAccessible *>(V_DISPATCH(&m_data[index]));
            if (long const role = kind & RoleMask)
            {
                VARIANT varRole;
                VariantInit(&varRole);
                child->get_accRole(varChild, &varRole);
                if (V_I4(&varRole) != role)
                    continue;
            }
            if (name)
            {
                BSTR strName = NULL;
                (child->*(kind & ByAccelerator ?
                    &IAccessible::get_accKeyboardShortcut :
                    &IAccessible::get_accName))(varChild, &strName);
                if (strName == NULL)
                    continue;
                int cmp = wcscmp(strName, name);
                SysFreeString(strName);
                if (cmp != 0)
                    continue;
            }
            VARIANT varRole;
            VariantInit(&varRole);
            child->get_accRole(varChild, &varRole);
            child->AddRef();
            return child;
        }
        return NULL;
    }
private:
    long const m_count;
    SAFEARRAY *const m_sa;
    VARIANT *const m_data;
    static long count(IAccessible *accessible)
    {
        long n = 0;
        if (accessible)
            accessible->get_accChildCount(&n);
        return n;
    }
    SAFEARRAY *sa()
    {
        return m_count ? SafeArrayCreateVector(VT_VARIANT, 0, m_count) : 0;
    }
    VARIANT *data()
    {
        void *p = NULL;
        SafeArrayAccessData(m_sa, &p);
        return static_cast<VARIANT *>(p);
    }
};
