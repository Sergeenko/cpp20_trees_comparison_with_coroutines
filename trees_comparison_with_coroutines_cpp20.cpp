#include <coroutine>
#include <stack>
#include <array>
#include <cassert>

template<typename T>
class gen
{
public:
    struct promise_type
    {
        using coro_handle = std::coroutine_handle<promise_type>;
        auto get_return_object() { return coro_handle::from_promise(*this); }
        auto initial_suspend() noexcept { return std::suspend_always(); }
        auto final_suspend() noexcept { return std::suspend_always(); }
        void unhandled_exception() {}
        auto yield_value(T value)
        {
            cur = std::move(value);
            return std::suspend_always();
        }
        void return_void()
        {
            return;
        }
        T cur;
    };

    using coro_handle = std::coroutine_handle<promise_type>;
    gen(coro_handle h) : handle(h) {}
    gen(const gen&) = delete;
    gen(gen&& other) : handle(other.handle) { other.handle = nullptr; }
    ~gen() { handle.destroy(); }

    bool is_done() { return handle.done(); }
    T next()
    {
        if (!is_done())
        {
            handle.resume();
            return std::move(handle.promise().cur);
        }
        return {};
    }
private:
    std::coroutine_handle<promise_type> handle;
};

template<typename T>
gen<T*> dfs_coro(T* p)
{
    std::stack<T*> s;
    s.push(p);
    while (not s.empty())
    {
        auto cur = s.top();
        s.pop();
        if (cur)
        {
            s.push(cur->r);
            s.push(cur->l);
        }
        co_yield cur;
    }
    co_return;
}

template<typename T, std::size_t Size>
class Tree
{
    struct TNode
    {
        T v{};
        TNode* l = nullptr;
        TNode* r = nullptr;
    };

public:
    Tree(std::array<T, Size> arr)
    {
        fill(std::move(arr));
        build();
    }

    Tree(std::array<T, Size> arr, T)
    {
        fill(std::move(arr));
        build_other_tree_with_same_order();
    }
    bool areSame(const Tree& other)
    {
        TNode* my_head = head;
        TNode* other_head = other.head;

        auto my_coro = dfs_coro<TNode>(my_head);
        auto other_coro = dfs_coro<TNode>(other_head);
        while ((my_head and other_head) or
            (!my_head and !other_head and (!my_coro.is_done() and !other_coro.is_done())))
        {
            if (my_head and other_head and my_head->v != other_head->v)
                return false;
            my_head = my_coro.next();
            other_head = other_coro.next();
        }
        return !my_head and !other_head;
    }

    TNode* head = nullptr;
private:
    void fill(std::array<T, Size> arr)
    {
        std::size_t idx = 0;
        for (auto&& e : arr)
        {
            t[idx++].v = std::move(e);
        }
    }
    void build()
    {
        t[0].l = &t[1]; t[0].r = &t[4];
        t[1].l = &t[2]; t[1].r = &t[3];
        t[4].l = &t[5];
        head = &t[0];
    }
    void build_other_tree_with_same_order()
    {
        t[0].r = &t[1];
        t[1].l = &t[2]; t[1].r = &t[3];
        t[3].l = &t[4]; t[3].r = &t[5];
        head = &t[0];
    }
    std::array<TNode, Size> t;
};

int main()
{
    auto treeValues = std::array{ 1,2,3,4,5,6 };
    Tree t1(treeValues);
    Tree t2(treeValues);
    assert(t1.areSame(t2));

    Tree t3(treeValues, {});
    assert(not t1.areSame(t3));
    assert(not t2.areSame(t3));
}
