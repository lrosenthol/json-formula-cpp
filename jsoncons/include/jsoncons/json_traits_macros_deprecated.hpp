// Copyright 2019 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSON_TRAITS_MACROS_DEPRECATED_HPP
#define JSONCONS_JSON_TRAITS_MACROS_DEPRECATED_HPP

#include <jsoncons/json_traits_macros.hpp>

#if !defined(JSONCONS_NO_DEPRECATED)

#define JSONCONS_MEMBER_TRAITS_DECL(ValueType, ...)  \
    JSONCONS_MEMBER_TRAITS_BASE(JSONCONS_AS,JSONCONS_TO_JSON,0, ValueType, JSONCONS_NARGS(__VA_ARGS__), 0, __VA_ARGS__) \
  /**/

#define JSONCONS_TPL_MEMBER_TRAITS_DECL(NumTemplateParams, ValueType, ...)  \
    JSONCONS_MEMBER_TRAITS_BASE(JSONCONS_AS,JSONCONS_TO_JSON,NumTemplateParams, ValueType, JSONCONS_NARGS(__VA_ARGS__), 0, __VA_ARGS__) \
  /**/

#define JSONCONS_MEMBER_NAMED_TRAITS_DECL(ValueType, ...)  \
    JSONCONS_MEMBER_NAME_TRAITS_BASE(JSONCONS_NAME_AS, JSONCONS_NAME_TO_JSON, 0, ValueType, JSONCONS_NARGS(__VA_ARGS__), 0, __VA_ARGS__) \
  /**/

#define JSONCONS_TPL_MEMBER_NAMED_TRAITS_DECL(NumTemplateParams, ValueType, ...)  \
    JSONCONS_MEMBER_NAME_TRAITS_BASE(JSONCONS_NAME_AS, JSONCONS_NAME_TO_JSON, NumTemplateParams, ValueType, JSONCONS_NARGS(__VA_ARGS__), 0, __VA_ARGS__) \
  /**/

#define JSONCONS_GETTER_SETTER_TRAITS_DECL(ValueType,GetPrefix,SetPrefix, ...)  \
    JSONCONS_GETTER_SETTER_TRAITS_BASE(JSONCONS_GETTER_SETTER_AS, JSONCONS_GETTER_SETTER_TO_JSON,0, ValueType,GetPrefix,SetPrefix, JSONCONS_NARGS(__VA_ARGS__), 0, __VA_ARGS__) \
  /**/

#define JSONCONS_TPL_GETTER_SETTER_TRAITS_DECL(NumTemplateParams, ValueType,GetPrefix,SetPrefix, ...)  \
    JSONCONS_GETTER_SETTER_TRAITS_BASE(JSONCONS_GETTER_SETTER_AS, JSONCONS_GETTER_SETTER_TO_JSON,NumTemplateParams, ValueType,GetPrefix,SetPrefix, JSONCONS_NARGS(__VA_ARGS__), 0, __VA_ARGS__) \
  /**/

#define JSONCONS_GETTER_SETTER_NAMED_TRAITS_DECL(ValueType, ...)  \
JSONCONS_GETTER_SETTER_NAME_TRAITS_BASE(JSONCONS_GETTER_SETTER_NAME_AS,JSONCONS_GETTER_SETTER_NAME_TO_JSON, 0, ValueType, JSONCONS_NARGS(__VA_ARGS__), 0, __VA_ARGS__) \
  /**/
 
#define JSONCONS_TPL_GETTER_SETTER_NAMED_TRAITS_DECL(NumTemplateParams, ValueType, ...)  \
JSONCONS_GETTER_SETTER_NAME_TRAITS_BASE(JSONCONS_GETTER_SETTER_NAME_AS,JSONCONS_GETTER_SETTER_NAME_TO_JSON, NumTemplateParams, ValueType, JSONCONS_NARGS(__VA_ARGS__), 0, __VA_ARGS__) \
  /**/

#define JSONCONS_ALL_GETTER_CTOR_TRAITS                          JSONCONS_ALL_CTOR_GETTER_TRAITS
#define JSONCONS_N_GETTER_CTOR_TRAITS                            JSONCONS_N_CTOR_GETTER_TRAITS
#define JSONCONS_TPL_ALL_GETTER_CTOR_TRAITS                      JSONCONS_TPL_ALL_CTOR_GETTER_TRAITS
#define JSONCONS_TPL_N_GETTER_CTOR_TRAITS                        JSONCONS_TPL_N_CTOR_GETTER_TRAITS

#define JSONCONS_ALL_GETTER_CTOR_NAME_TRAITS                     JSONCONS_ALL_CTOR_GETTER_NAME_TRAITS
#define JSONCONS_N_GETTER_CTOR_NAME_TRAITS                       JSONCONS_N_CTOR_GETTER_NAME_TRAITS
#define JSONCONS_TPL_ALL_GETTER_CTOR_NAME_TRAITS                 JSONCONS_TPL_ALL_CTOR_GETTER_NAME_TRAITS
#define JSONCONS_TPL_N_GETTER_CTOR_NAME_TRAITS                   JSONCONS_TPL_N_CTOR_GETTER_NAME_TRAITS

#define JSONCONS_PROPERTY_TRAITS_DECL                            JSONCONS_GETTER_SETTER_TRAITS_DECL
#define JSONCONS_TPL_PROPERTY_TRAITS_DECL                        JSONCONS_TPL_GETTER_SETTER_TRAITS_DECL
#define JSONCONS_TYPE_TRAITS_DECL                                JSONCONS_MEMBER_TRAITS_DECL
#define JSONCONS_MEMBER_TRAITS_NAMED_DECL                        JSONCONS_MEMBER_NAMED_TRAITS_DECL                   
#define JSONCONS_TEMPLATE_MEMBER_TRAITS_NAMED_DECL               JSONCONS_TPL_MEMBER_NAMED_TRAITS_DECL               
#define JSONCONS_TEMPLATE_GETTER_SETTER_TRAITS_NAMED_DECL        JSONCONS_TPL_GETTER_SETTER_NAMED_TRAITS_DECL        
#define JSONCONS_TEMPLATE_MEMBER_TRAITS_DECL                     JSONCONS_TPL_MEMBER_TRAITS_DECL                     

#define JSONCONS_N_MEMBER_NAMED_TRAITS              JSONCONS_N_MEMBER_NAME_TRAITS
#define JSONCONS_TPL_N_MEMBER_NAMED_TRAITS          JSONCONS_TPL_N_MEMBER_NAME_TRAITS
#define JSONCONS_ALL_MEMBER_NAMED_TRAITS            JSONCONS_ALL_MEMBER_NAME_TRAITS
#define JSONCONS_TPL_ALL_MEMBER_NAMED_TRAITS        JSONCONS_TPL_ALL_MEMBER_NAME_TRAITS

#define JSONCONS_ALL_GETTER_CTOR_NAMED_TRAITS       JSONCONS_ALL_GETTER_CTOR_NAME_TRAITS
#define JSONCONS_TPL_ALL_GETTER_CTOR_NAMED_TRAITS   JSONCONS_TPL_ALL_GETTER_CTOR_NAME_TRAITS
#define JSONCONS_N_GETTER_CTOR_NAMED_TRAITS         JSONCONS_N_GETTER_CTOR_NAME_TRAITS
#define JSONCONS_TPL_N_GETTER_CTOR_NAMED_TRAITS     JSONCONS_TPL_N_GETTER_CTOR_NAME_TRAITS

#define JSONCONS_ENUM_NAMED_TRAITS                  JSONCONS_ENUM_NAME_TRAITS

#define JSONCONS_N_GETTER_SETTER_NAMED_TRAITS       JSONCONS_N_GETTER_SETTER_NAME_TRAITS
#define JSONCONS_TPL_N_GETTER_SETTER_NAMED_TRAITS   JSONCONS_TPL_N_GETTER_SETTER_NAME_TRAITS
#define JSONCONS_ALL_GETTER_SETTER_NAMED_TRAITS     JSONCONS_ALL_GETTER_SETTER_NAME_TRAITS
#define JSONCONS_TPL_ALL_GETTER_SETTER_NAMED_TRAITS JSONCONS_TPL_ALL_GETTER_SETTER_NAME_TRAITS

#define JSONCONS_N_MEMBER_TRAITS_DECL                            JSONCONS_N_MEMBER_TRAITS
#define JSONCONS_TPL_N_MEMBER_TRAITS_DECL                        JSONCONS_TPL_N_MEMBER_TRAITS
#define JSONCONS_ALL_MEMBER_TRAITS_DECL                          JSONCONS_ALL_MEMBER_TRAITS
#define JSONCONS_TPL_ALL_MEMBER_TRAITS_DECL                      JSONCONS_TPL_ALL_MEMBER_TRAITS 
#define JSONCONS_N_MEMBER_NAMED_TRAITS_DECL                      JSONCONS_N_MEMBER_NAMED_TRAITS
#define JSONCONS_TPL_N_MEMBER_NAMED_TRAITS_DECL                  JSONCONS_TPL_N_MEMBER_NAMED_TRAITS
#define JSONCONS_ALL_MEMBER_NAMED_TRAITS_DECL                    JSONCONS_ALL_MEMBER_NAMED_TRAITS
#define JSONCONS_TPL_ALL_MEMBER_NAMED_TRAITS_DECL                JSONCONS_TPL_ALL_MEMBER_NAMED_TRAITS
#define JSONCONS_ALL_GETTER_CTOR_TRAITS_DECL                     JSONCONS_ALL_GETTER_CTOR_TRAITS
#define JSONCONS_TPL_ALL_GETTER_CTOR_TRAITS_DECL                 JSONCONS_TPL_ALL_GETTER_CTOR_TRAITS
#define JSONCONS_N_GETTER_CTOR_TRAITS_DECL                       JSONCONS_N_GETTER_CTOR_TRAITS
#define JSONCONS_N_ALL_GETTER_CTOR_TRAITS_DECL                   JSONCONS_N_ALL_GETTER_CTOR_TRAITS
#define JSONCONS_ALL_GETTER_CTOR_NAMED_TRAITS_DECL               JSONCONS_ALL_GETTER_CTOR_NAMED_TRAITS
#define JSONCONS_TPL_ALL_GETTER_CTOR_NAMED_TRAITS_DECL           JSONCONS_TPL_ALL_GETTER_CTOR_NAMED_TRAITS
#define JSONCONS_N_GETTER_CTOR_NAMED_TRAITS_DECL                 JSONCONS_N_GETTER_CTOR_NAMED_TRAITS
#define JSONCONS_TPL_N_GETTER_CTOR_NAMED_TRAITS_DECL             JSONCONS_TPL_N_GETTER_CTOR_NAMED_TRAITS
#define JSONCONS_ENUM_TRAITS_DECL                                JSONCONS_ENUM_TRAITS
#define JSONCONS_ENUM_NAMED_TRAITS_DECL                          JSONCONS_ENUM_NAMED_TRAITS
#define JSONCONS_N_GETTER_SETTER_TRAITS_DECL                     JSONCONS_N_GETTER_SETTER_TRAITS
#define JSONCONS_TPL_N_GETTER_SETTER_TRAITS_DECL                 JSONCONS_TPL_N_GETTER_SETTER_TRAITS
#define JSONCONS_ALL_GETTER_SETTER_TRAITS_DECL                   JSONCONS_ALL_GETTER_SETTER_TRAITS
#define JSONCONS_TPL_ALL_GETTER_SETTER_TRAITS_DECL               JSONCONS_TPL_ALL_GETTER_SETTER_TRAITS
#define JSONCONS_N_GETTER_SETTER_NAMED_TRAITS_DECL               JSONCONS_N_GETTER_SETTER_NAMED_TRAITS
#define JSONCONS_TPL_N_GETTER_SETTER_NAMED_TRAITS_DECL           JSONCONS_TPL_N_GETTER_SETTER_NAMED_TRAITS
#define JSONCONS_ALL_GETTER_SETTER_NAMED_TRAITS_DECL             JSONCONS_ALL_GETTER_SETTER_NAMED_TRAITS
#define JSONCONS_TPL_ALL_GETTER_SETTER_NAMED_TRAITS_DECL         JSONCONS_TPL_ALL_GETTER_SETTER_NAMED_TRAITS
#define JSONCONS_POLYMORPHIC_TRAITS_DECL                         JSONCONS_POLYMORPHIC_TRAITS
#define JSONCONS_NONDEFAULT_MEMBER_TRAITS_DECL                   JSONCONS_ALL_MEMBER_TRAITS
#define JSONCONS_TEMPLATE_STRICT_MEMBER_TRAITS_DECL              JSONCONS_TPL_ALL_MEMBER_TRAITS

#define JSONCONS_STRICT_MEMBER_TRAITS_NAMED_DECL                 JSONCONS_ALL_MEMBER_NAME_TRAITS            
#define JSONCONS_STRICT_TEMPLATE_MEMBER_TRAITS_DECL              JSONCONS_TPL_ALL_MEMBER_TRAITS              
#define JSONCONS_STRICT_TEMPLATE_MEMBER_TRAITS_NAMED_DECL        JSONCONS_TPL_ALL_MEMBER_NAME_TRAITS        
#define JSONCONS_ENUM_TRAITS_NAMED_DECL                          JSONCONS_ENUM_NAME_TRAITS                     
#define JSONCONS_GETTER_CTOR_TRAITS_NAMED_DECL                   JSONCONS_ALL_GETTER_CTOR_NAME_TRAITS              
#define JSONCONS_TEMPLATE_GETTER_CTOR_TRAITS_DECL                JSONCONS_TPL_ALL_GETTER_CTOR_TRAITS                
#define JSONCONS_TEMPLATE_GETTER_CTOR_TRAITS_NAMED_DECL          JSONCONS_TPL_ALL_GETTER_CTOR_NAME_TRAITS          
#define JSONCONS_GETTER_SETTER_TRAITS_NAMED_DECL                 JSONCONS_ALL_GETTER_SETTER_NAME_TRAITS            
#define JSONCONS_STRICT_GETTER_SETTER_TRAITS_NAMED_DECL          JSONCONS_ALL_GETTER_SETTER_NAME_TRAITS     
#define JSONCONS_STRICT_TEMPLATE_GETTER_SETTER_TRAITS_NAMED_DECL JSONCONS_TPL_ALL_GETTER_SETTER_NAME_TRAITS 
#define JSONCONS_STRICT_TPL_MEMBER_TRAITS_DECL                   JSONCONS_TPL_ALL_MEMBER_TRAITS
#define JSONCONS_STRICT_TPL_MEMBER_NAMED_TRAITS_DECL             JSONCONS_TPL_ALL_MEMBER_NAME_TRAITS
#define JSONCONS_STRICT_TPL_GETTER_SETTER_NAMED_TRAITS_DECL      JSONCONS_TPL_ALL_GETTER_SETTER_NAME_TRAITS

#define JSONCONS_STRICT_MEMBER_TRAITS_DECL                       JSONCONS_ALL_MEMBER_TRAITS 
#define JSONCONS_TPL_STRICT_MEMBER_TRAITS_DECL                   JSONCONS_TPL_ALL_MEMBER_TRAITS
#define JSONCONS_STRICT_MEMBER_NAMED_TRAITS_DECL                 JSONCONS_ALL_MEMBER_NAME_TRAITS
#define JSONCONS_TPL_STRICT_MEMBER_NAMED_TRAITS_DECL             JSONCONS_ALL_STRICT_MEMBER_NAME_TRAITS
#define JSONCONS_STRICT_PROPERTY_TRAITS_DECL                     JSONCONS_ALL_GETTER_SETTER_TRAITS
#define JSONCONS_TPL_STRICT_PROPERTY_TRAITS_DECL                 JSONCONS_TPL_ALL_GETTER_SETTER_TRAITS
#define JSONCONS_STRICT_GETTER_SETTER_NAMED_TRAITS_DECL          JSONCONS_ALL_GETTER_SETTER_NAME_TRAITS
#define JSONCONS_TPL_STRICT_GETTER_SETTER_NAMED_TRAITS_DECL      JSONCONS_TPL_ALL_GETTER_SETTER_NAME_TRAITS
#define JSONCONS_GETTER_CTOR_TRAITS_DECL                         JSONCONS_ALL_GETTER_CTOR_TRAITS
#define JSONCONS_TPL_GETTER_CTOR_TRAITS_DECL                     JSONCONS_TPL_ALL_GETTER_CTOR_TRAITS  
#define JSONCONS_GETTER_CTOR_NAMED_TRAITS_DECL                   JSONCONS_ALL_GETTER_CTOR_NAME_TRAITS
#define JSONCONS_TPL_GETTER_CTOR_NAMED_TRAITS_DECL               JSONCONS_TPL_ALL_GETTER_CTOR_NAME_TRAITS
#define JSONCONS_N_PROPERTY_TRAITS_DECL                          JSONCONS_N_GETTER_SETTER_TRAITS
#define JSONCONS_ALL_PROPERTY_TRAITS_DECL                        JSONCONS_ALL_GETTER_SETTER_TRAITS
#define JSONCONS_TPL_N_PROPERTY_TRAITS_DECL                      JSONCONS_TPL_N_GETTER_SETTER_TRAITS
#define JSONCONS_TPL_ALL_PROPERTY_TRAITS_DECL                    JSONCONS_TPL_ALL_GETTER_SETTER_TRAITS

#endif

#endif
