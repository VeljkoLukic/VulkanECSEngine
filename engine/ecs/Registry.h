#pragma once
#include "engine/ecs/ComponentPool.h"
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <queue>

namespace ecs {

template<typename... Ts>
class View {
    static_assert(sizeof...(Ts) > 0, "View requires at least one component type");
    std::tuple<ComponentPool<Ts>*...> m_pools;

public:
    explicit View(ComponentPool<Ts>&... pools) : m_pools(&pools...) {}

    template<typename Fn>
    void each(Fn&& fn) {
        auto& primary = *std::get<0>(m_pools);
        for (std::size_t i = 0; i < primary.size(); ++i) {
            const EntityID entity = primary.entities()[i];
            if (hasAll<1>(entity))
                fn(entity, std::get<ComponentPool<Ts>*>(m_pools)->get(entity)...);
        }
    }

private:
    template<std::size_t I>
    bool hasAll(EntityID entity) const {
        if constexpr (I >= sizeof...(Ts)) return true;
        else return std::get<I>(m_pools)->has(entity) && hasAll<I + 1>(entity);
    }
};

class Registry {
public:
    [[nodiscard]] EntityID create() {
        if (!m_free.empty()) {
            EntityID id = m_free.front();
            m_free.pop();
            return id;
        }
        return m_next++;
    }

    void destroy(EntityID entity) {
        for (auto& [_, pool] : m_pools)
            if (pool->has(entity)) pool->remove(entity);
        m_free.push(entity);
    }

    template<typename T>
    T& emplace(EntityID entity, T component = {}) {
        return pool<T>().add(entity, std::move(component));
    }

    template<typename T>
    void remove(EntityID entity) { pool<T>().remove(entity); }

    template<typename T>
    [[nodiscard]] T& get(EntityID entity) { return pool<T>().get(entity); }

    template<typename T>
    [[nodiscard]] const T& get(EntityID entity) const {
        return cpool<T>().get(entity);
    }

    template<typename T>
    [[nodiscard]] bool has(EntityID entity) const {
        auto it = m_pools.find(std::type_index(typeid(T)));
        return it != m_pools.end() && it->second->has(entity);
    }

    template<typename... Ts>
    [[nodiscard]] View<Ts...> view() { return View<Ts...>(pool<Ts>()...); }

private:
    template<typename T>
    ComponentPool<T>& pool() {
        auto key = std::type_index(typeid(T));
        auto [it, inserted] = m_pools.emplace(key, nullptr);
        if (inserted) it->second = std::make_unique<ComponentPool<T>>();
        return static_cast<ComponentPool<T>&>(*it->second);
    }

    template<typename T>
    const ComponentPool<T>& cpool() const {
        return static_cast<const ComponentPool<T>&>(*m_pools.at(std::type_index(typeid(T))));
    }

    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_pools;
    std::queue<EntityID> m_free;
    EntityID             m_next = 0;
};

} // namespace ecs
