//
// Created by 58226 on 2023/5/6.
//

#ifndef OPTIONNALCHAIN_H
#define OPTIONNALCHAIN_H
#include <QHash>
#include <QList>
#include <QMap>
#include <QSet>
template<typename, typename>
class MapOperation;
template<typename>
class SourcesOperation;
template<typename>
class Optional;

template<typename T>
///单项操作
class Optional
{
public:
    inline static Optional<T> of(T obj)
    {
        return obj == nullptr ? empty() : Optional<T>(obj);
    }

    static inline Optional<T> empty()
    {
        return Optional<T>();
    }

private:
    Optional() { m_data = nullptr; };
    explicit Optional(const T &data) : m_data(std::move(data)) {}

public:
    Optional<T> &operator=(const T &other)
    {
        m_data = other;
        return *this;
    }

    T &operator()(T)
    {
        return std::move(m_data);
    }

    bool operator==(const Optional<T> &other)
    {
        return m_data == other.m_data;
    }
public:
    ///获取源
    T get()
    {
        return std::move(m_data);
    }
    ///判断是否为默认值
    inline bool isEmpty() const
    {
        return this->m_data == nullptr;
    }

    template<typename Res>
    inline Optional<Res> map(const std::function<Res(T)> &lambda)
    {
        if (isEmpty())
            return Optional<Res>::empty();
        auto curObj = lambda(m_data);
        return curObj == nullptr ? Optional<Res>::empty() : Optional<Res>::of(curObj);
    }

    template<typename Res>
    inline Optional<Res> map(Res (*func)(T))
    {
        if (isEmpty())
            return Optional<Res>::empty();
        auto curObj = func(m_data);
        return curObj == nullptr ? Optional<Res>::empty() : Optional<Res>::of(curObj);
    }

    template<typename F, typename Res>
    inline Optional<Res> map(F func)
    {
        if (isEmpty())
            return Optional<Res>::empty();
        auto curObj = func(m_data);
        return curObj == nullptr ? Optional<Res>::empty() : Optional<Res>::of(curObj);
    }

    template<typename Res>
    inline Optional<Res> flatMap(const std::function<Optional<Res>(T)> &lambda)
    {
        if (isEmpty())
            return Optional<Res>::empty();
        return lambda(m_data);
    }
    template<typename Res>
    inline Optional<Res> flatMap(Res(*func(T)))
    {
        if (isEmpty())
            return Optional<Res>::empty();
        return func(m_data);
    }
    template<typename F, typename Res>
    inline Optional<Res> flatMap(F callable)
    {
        if (isEmpty())
            return Optional<Res>::empty();
        return lambda(m_data);
    }


    template<typename Res>
    inline Res orElse(const Res &r)
    {
        return this->isEmpty() ? r : m_data;
    }

    inline Optional<T> filter(const std::function<bool(T)> &fi)
    {
        return (isEmpty() || !fi(m_data)) ? empty() : (*this);
    }
    inline Optional<T> filter(bool(*func(T)))
    {
        return (isEmpty() || !func(m_data)) ? empty() : (*this);
    }
    template<typename F>
    inline Optional<T> filter(F fi)
    {
        return (isEmpty() || !fi(m_data)) ? empty() : (*this);
    }

    inline T orElseGet(const std::function<T(void)> &func)
    {
        return isEmpty() ? func() : m_data;
    }

public:
    ///单项执行某种操作
    Optional<T> &then(const std::function<void(T &)> &action)
    {
        if (m_data != nullptr) {
            action(m_data);
        }
        return *this;
    }
    Optional<T> &then(void(*action(T)))
    {
        if (m_data != nullptr) {
            action(m_data);
        }
        return *this;
    }
    template<typename F>
    Optional<T> &then(F action)
    {
        if (m_data != nullptr) {
            action(m_data);
        }
        return *this;
    }

    ///变更为操作集合
    SourcesOperation<T> groupedWith(const T &other)
    {
        return std::move(SourcesOperation<T>({std::move(m_data), other}));
    }


private:
    T m_data;
};

template<typename T>
///集合操作
class SourcesOperation
{
public:
    SourcesOperation(const QList<T> &source) : m_sourcesCache(std::move(source)) {}

public:
    ///获取源数据
    QList<T> &get()
    {
        return m_sourcesCache;
    }
    ///delegate for insert
    SourcesOperation<T> &insert(const T &data, int index = -1)
    {
        if (index < 0) {
            m_sourcesCache.push_back(data);
        } else {
            m_sourcesCache.insert(index, data);
        }
        return *this;
    }
    ///delegate for size;
    int count()
    {
        return m_sourcesCache.count();
    }
    ///delegate for empty check
    bool isEmpty()
    {
        return m_sourcesCache.isEmpty();
    }

public:
    ///delegate for operator =
    Optional<T> &operator[](int index)
    {
        if (index < 0 || index >= m_sourcesCache.length())
            return {nullptr};
        return std::move({m_sourcesCache[index]});
    }
    ///parse to QList
    QList<T> &operator()(QList<T>)
    {
        return std::move(m_sourcesCache);
    }
    ///relace inner data
    SourcesOperation<T> &operator=(const QList<T> &otherSources)
    {
        m_sourcesCache = otherSources;
        return *this;
    }

    SourcesOperation<T> operator+(const T &other)
    {
        m_sourcesCache.append(other);
        return SourcesOperation<T>(QList<T>(m_sourcesCache));
    }
    SourcesOperation<T> operator+(const Optional<T> &other)
    {
        m_sourcesCache.append(other);
        return SourcesOperation<T>(QList<T>(m_sourcesCache));
    }
    SourcesOperation<T> operator+(const QList<T> &otherSources)
    {
        m_sourcesCache.append(otherSources);
        return SourcesOperation<T>(QList<T>(m_sourcesCache));
    }
    SourcesOperation<T> operator+(const SourcesOperation<T> &otherSources)
    {
        m_sourcesCache.append(otherSources.m_sourcesCache);
        return SourcesOperation<T>(QList<T>(m_sourcesCache));
    }

    SourcesOperation<T> &operator+=(const T &other)
    {
        m_sourcesCache.append(other);
        return *this;
    }
    SourcesOperation<T> &operator+=(const Optional<T> &other)
    {
        m_sourcesCache.append(other);
        return *this;
    }
    SourcesOperation<T> &operator+=(const QList<T> &otherSources)
    {
        m_sourcesCache.append(otherSources);
        return *this;
    }
    SourcesOperation<T> &operator+=(const SourcesOperation<T> &otherSources)
    {
        m_sourcesCache.append(otherSources.m_sourcesCache);
        return *this;
    }

    bool operator==(const QList<T> &otherSources) const
    {
        return isEqual(otherSources);
    }
    bool operator==(const SourcesOperation<T> &otherSources) const
    {
        return isEqual(otherSources);
    }

public:
    ///使集合的每一项进行操作
    SourcesOperation<T> &forEach(std::function<void(T &)> action)
    {
        for (auto &itr : m_sourcesCache) {
            action(itr);
        }
        return *this;
    }

    ///反转集合
    SourcesOperation<T> reverse()
    {
        return {reversed(m_sourcesCache)};
    }

    template<typename KeyType>
    ///变更为mapOption,需要提供获取Key值的方法
    MapOperation<KeyType, T> &asMapOperation(std::function<KeyType(T)> getKey, std::function<bool(KeyType)> checkSuccess)
    {
        QHash<KeyType, T> curSource;
        for (auto &itr : m_sourcesCache) {
            auto curKey = getKey(itr);
            if (checkSuccess(curKey)) {
                curSource.insert(curKey, itr);
            }
        }
        return MapOperation(curSource);
    }

    ///变更为SingleOperation的集合
    SourcesOperation<Optional<T>> &asOperations()
    {
        SourcesOperation<Optional<T>> res;
        for (const auto &itr : m_sourcesCache) {
            res.insert(Optional<T>(itr));
        }
        return res;
    }

    ///判断两个集合相等
    bool isEqual(const QList<T> &other)
    {
        if (m_sourcesCache.count() != other.count())
            return false;
        if (m_sourcesCache.isEmpty() && other.isEmpty())
            return true;
        for (int i = m_sourcesCache.count() - 1; i >= 0; i--) {
            if (m_sourcesCache[i] != other[i])
                return false;
        }
        return true;
    }
    ///判断集合相等
    bool isEqual(const SourcesOperation<T> &other)
    {
        if (m_sourcesCache.count() != other.count())
            return false;
        if (m_sourcesCache.isEmpty() && other.isEmpty())
            return true;
        for (int i = m_sourcesCache.count() - 1; i >= 0; i--) {
            if (m_sourcesCache[i] != other[i])
                return false;
        }
        return true;
    }
    ///获取与另一个集合的差值
    SourcesOperation<T> subtract(const QList<T> &other)
    {
        QList<T> res;
        for (int i = 0; i < m_sourcesCache.count() && i < other.count(); i++) {
            if (m_sourcesCache[i] == other[i]) {
                continue;
            }
            for (int j = i; i < m_sourcesCache.count(); j++) {
                res.push_back(m_sourcesCache[j]);
                break;
            }
        }
        return res;
    }

public:
    ///集合的第一项，可能为空
    Optional<T> first()
    {
        return m_sourcesCache.empty() ? Optional<T>::empty() : Optional<T>::of(m_sourcesCache.first());
    }
    ///集合的最后一项，可能为空
    Optional<T> last()
    {
        return m_sourcesCache.empty() ? Optional<T>::empty() : Optional<T>::of(m_sourcesCache.last());
    }

    ///找到集合中第一个满足条件的项
    Optional<T> firstOf(const std::function<bool(const T &)> &matchFunction, const T &defaultValue)
    {
        T result = defaultValue;
        auto begin = m_sourcesCache.begin();
        while (begin != m_sourcesCache.end()) {
            if (matchFunction(*begin)) {
                result = *begin;
                break;
            }
            ++begin;
        }
        return Optional<T>::of(result);
    }

    ///找到集合中最后一个满足条件的项
    Optional<T> lastOf(std::function<bool(T)> MatchFunction, const T defaultValue)
    {
        T result = defaultValue;
        auto begin = m_sourcesCache.begin();
        while (begin != m_sourcesCache.end()) {
            if (MatchFunction(*begin)) {
                result = *begin;
            }
            ++begin;
        }
        return Optional<T>::of(result);
    }

    ///找到集合中所有满足条件的项
    SourcesOperation<T> where(std::function<bool(T)> MatchFunction)
    {
        QList<T> findResult;
        auto begin = m_sourcesCache.begin();
        while (begin != m_sourcesCache.end()) {
            if (MatchFunction(*begin)) {
                findResult.push_back(*begin);
            }
            ++begin;
        }
        return findResult;
    }

    template<typename Res>
    ///从集合中获取某些内容
    SourcesOperation<Res> select(std::function<Res(const T &)> selectFunc)
    {
        QList<Res> res;
        for (auto &i : m_sourcesCache) {
            res.push_back(selectFunc(i));
        }
        return res;
    }

    template<typename Res>
    ///从集合中获取某些内容，通过提供的内容校验筛选内容
    SourcesOperation<Res> select(std::function<Res(const T &)> selectFunc, std::function<bool(const Res &)> checkSuccess)
    {
        QList<Res> res;
        for (auto &i : m_sourcesCache) {
            auto curRes = selectFunc(i);
            if (checkSuccess(curRes)) {
                res.append(curRes);
            }
        }
        return res;
    }

    ///判断集合中是否存在满足条件的一项
    bool any(std::function<bool(const T &)> matchFunc)
    {
        for (auto &itr : m_sourcesCache) {
            if (matchFunc(itr))
                return true;
        }
        return false;
    }

    ///判断集合中是否所有项都满足指定条件
    bool all(const std::function<bool(const T &)> &matchFunc)
    {
        for (const auto &itr : m_sourcesCache) {
            if (!matchFunc(itr))
                return false;
        }
        return true;
    }

    ///跳过指定数目
    SourcesOperation<T> skip(int skipCount)
    {
        QList<T> result = QList<T>();

        if (skipCount >= m_sourcesCache.size())
            return {};
        for (int i = skipCount; i < m_sourcesCache.size(); i++) {
            result.push_back(m_sourcesCache[i]);
        }
        return {result};
    }

    ///获取指定数目
    SourcesOperation<T> take(int takeCount)
    {
        QList<T> result = QList<T>();

        if (takeCount >= m_sourcesCache.size())
            return {m_sourcesCache};
        for (int i = 0; i < takeCount; i++) {
            result.push_back(m_sourcesCache[i]);
        }
        return {result};
    }

    ///集合中的最大值
    T max()
    {
        return aggregate<T>(m_sourcesCache.first(), [](const T &first, const T &second) {
            return std::fmax(first, second);
        });
    }

    ///集合中的最小值
    T min()
    {
        return aggregate<T>(m_sourcesCache.first(), [](const T &first, const T &second) {
            return std::fmin(first, second);
        });
    }

    ///集合中的最大值
    template<typename Res>
    T max(const std::function<Res(const T &l)> &getter)
    {
        T &curElement = m_sourcesCache.first();
        Res curMaxRes = getter(curElement);
        for (const auto &itr : m_sourcesCache) {
            if (curElement == itr)
                continue;
            auto curRes = getter(itr);
            if (curRes > curMaxRes) {
                curElement = itr;
                curMaxRes = curRes;
            }
        }
        return curElement;
    }

    ///集合中的最小值
    template<typename Res>
    T min(const std::function<Res(const T &l)> &getter)
    {
        T &curElement = m_sourcesCache.first();
        Res curMinRes = getter(curElement);
        for (const auto &itr : m_sourcesCache) {
            if (curElement == itr)
                continue;
            auto curRes = getter(itr);
            if (curRes < curMinRes) {
                curElement = itr;
                curMinRes = curRes;
            }
        }
        return curElement;
    }

    template<typename Res>
    ///获取集合中指定最大值
    Res maxOf(const std::function<Res(const T &)> &getter, Res defaultValue)
    {
        Res res = defaultValue;
        for (const auto &itr : m_sourcesCache) {
            auto curData = getter(itr);
            if (curData > res)
                res = curData;
        }
        return res;
    }

    template<typename Res>
    ///获取集合中指定最小值
    Res minOf(const std::function<Res(const T &)> &getter, Res defaultValue)
    {
        Res res = defaultValue;
        for (const auto &itr : m_sourcesCache) {
            auto curData = getter(itr);
            if (curData < res)
                res = curData;
        }
        return res;
    }

    ///集合聚合
    template<typename Res>
    inline Res aggregate(const Res &startSeed, const std::function<Res(const Res &, const T &)> &func)
    {
        Res result = startSeed;
        for (const auto &itr : m_sourcesCache) {
            result = func(result, itr);
        }
        return result;
    }


    ///获取集合内满足条件的数量
    inline int countOf(const std::function<bool(const T &)> &checker)
    {
        return aggregate<int>(0, [&](int currentCount, const T &curObj) {
            return currentCount + (checker(curObj) ? 1 : 0);
        });
    };

    ///计算集合内总值
    template<typename Res>
    inline Res sum(const std::function<Res(const T &)> &getter, const Res &defaultValue)
    {
        return aggregate<Res>(defaultValue, [&](const T &curObj) {
            return defaultValue + getter(curObj);
        });
    }


    ///集合分组
    template<typename KeyType>
    inline MapOperation<KeyType, QList<T>> groupBy(const std::function<KeyType(const T &)> &keySelector)
    {
        return {aggregate<QHash<KeyType, QList<T>>>(QHash<KeyType, QList<T>>(),
                                                    [&](QHash<KeyType, QList<T>> &groupedElements,
                                                        const T &element) {
                                                        KeyType key = keySelector(element);
                                                        if (!groupedElements.contains(key))
                                                            groupedElements.insert(key, QList<T>());
                                                        groupedElements[key].pushBack(element);
                                                        return groupedElements;
                                                    })};
    }


public:
    ///debug use to check collection
    inline QString toString(char spliter = ',')
    {
        QString res;
        for (const auto &itr : m_sourcesCache) {
            res += QString("%1%2").arg(itr).arg(spliter);
        }
        if (!res.isEmpty()) {
            res.chop(1);
        }
        return res;
    }

private:
    QList<T> m_sourcesCache;
};

template<typename K, typename T>
///键值对操作
class MapOperation
{
public:
    explicit MapOperation(const QHash<K, T> &mapSource) : m_map(std::move(mapSource)) {}
    explicit MapOperation(const std::unordered_map<K, T> &mapsource)
    {
        m_map = QHash<K, T>(mapsource.begin(), mapsource.end());
    }
    explicit MapOperation(const QMap<K, T> &mapsource)
    {
        m_map = QHash<K, T>(mapsource.begin(), mapsource.end());
    }
    explicit MapOperation(const std::map<K, T> &mapsource)
    {
        m_map = QHash<K, T>(mapsource.begin(), mapsource.end());
    }

public:
    MapOperation<K, T> &operator=(const MapOperation<K, T> &other)
    {
        m_map = other.m_map;
        return *this;
    }


    Optional<T> &operator[](const K &key)
    {
        if (m_map.contains(key)) {
            return Optional<T>::of(std::move(m_map[key]));
        }
        return Optional<T>::empty();
    }

    QHash<K, T> &operator()(QHash<K, T>)
    {
        return std::move(this->m_map);
    }

public:
    ///获取源
    QHash<K, T> &getData()
    {
        return std::move(m_map);
    }

public:
    ///delegate
    bool contains(K key)
    {
        return m_map.contains(key);
    }

    ///找到Map中值满足条件的第一个key
    SourcesOperation<K> where(std::function<bool(const T &curItr)> matchFunc)
    {
        QList<K> res;
        for (const auto &x : m_map.keys()) {
            if (matchFunc(m_map[x])) {
                res.push_back(x);
            }
        }
        return {res};
    }
    ///所有键
    SourcesOperation<K> keys()
    {
        return {std::move(m_map.keys())};
    }
    ///所有值
    SourcesOperation<T> values()
    {
        return {std::move(m_map.values())};
    }

    QHash<T, K> valueKeyMap()
    {
        QHash<T, K> res;
        for (auto itr : m_map.keys()) {
            auto curV = m_map[itr];
            if (res.contains(curV))
                continue;
            res.insert(curV, itr);
        }
        return res;
    }

private:
    QHash<K, T> m_map;
};


#pragma region methods
template<typename T>
QList<T> reversed(const QList<T> &in)
{
    QList<T> result;
    result.reserve(in.size());
    std::reverse_copy(in.begin(), in.end(), std::back_inserter(result));
    return result;
}

template<typename T>
///将集合变更为带一系列操作的对象
constexpr SourcesOperation<T> from(const QList<T> &sources)
{
    return SourcesOperation(sources);
}

template<typename T>
///将集合变更为带一系列操作的对象
constexpr SourcesOperation<T> from(const QVector<T> &sources)
{
    return SourcesOperation(sources.toList());
}

template<typename T>
///拿到对单个对象的操作
constexpr Optional<T> option(const T &singleObj)
{
    return Optional<T>::of(singleObj);
}

template<typename K, typename T>
///将HashMap变更为带一系列操作的对象
constexpr MapOperation<K, T> from(const QHash<K, T> &maps)
{
    return MapOperation<K, T>(maps);
}

template<typename K, typename T>
///将HashMap变更为带一系列操作的对象
constexpr MapOperation<K, T> from(const std::unordered_map<K, T> &maps)
{
    return MapOperation<K, T>(maps);
}

template<typename K, typename T>
///Qmap转换为一系列的操作对象
constexpr MapOperation<K, T> from(const QMap<K, T> &maps)
{
    QHash<K, T> curHashMap;
    auto start = maps.begin();
    while (start != maps.end()) {
        curHashMap.insert(start.key(), start.value());
        ++start;
    }
    return from(curHashMap);
}
#pragma endregion
#endif//OPTIONNALCHAIN_H
