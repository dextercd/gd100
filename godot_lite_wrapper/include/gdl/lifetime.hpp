#ifndef GDL_LIFETIME_HPP
#define GDL_LIFETIME_HPP

namespace gdl {

template<class T>
struct native_handle_funcs;

template<class T>
class lifetime {
protected:
    T m_native_handle;
    using funcs = native_handle_funcs<T>;

public:
    lifetime()
        : m_native_handle{funcs::new_default()}
    {
    }

    lifetime(T native_handle)
        : m_native_handle{native_handle}
    {
    }

    lifetime(lifetime const& other)
        : lifetime(funcs::new_copy(other.m_native_handle))
    {
    }

    lifetime(lifetime&& other)
        : lifetime(other.release())
    {
    }

    lifetime& operator=(lifetime const& other)
    {
        auto copy = other;
        std::swap(m_native_handle, copy.m_native_handle);
        return *this;
    }

    lifetime& operator=(lifetime&& other)
    {
        std::swap(m_native_handle, other.m_native_handle);
        other = lifetime{};
        return *this;
    }

    ~lifetime()
    {
        funcs::destroy(m_native_handle);
    }

    T release()
    {
        auto const ret = m_native_handle;
        m_native_handle = funcs::new_default();
        return ret;
    }

    T* get() noexcept
    {
        return &m_native_handle;
    }

    T const* get() const noexcept
    {
        return &m_native_handle;
    }
};

} // gdl::

#endif // header guard
