#ifndef XUDEFINE_H
#define XUDEFINE_H
#define InperQtPropertyDefine(type, name, access_permission)\
access_permission:\
    type name;\
    public:\
    inline void set##name(type v) {\
        name = v;\
}\
    inline type get##name() {\
        return name;\
}\

#endif // XUDEFINE_H
