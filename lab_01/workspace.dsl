workspace {
    name "Сервис доставки"
    description "Простая система управления сервисом доставки, позволяющая оперировать посылками и информацией о них"

    # включаем режим с иерархической системой идентификаторов
    !identifiers hierarchical

    # Модель архитектуры
    model {

        # Настраиваем возможность создания вложенных груп
        properties { 
            structurizr.groupSeparator "/"
        }
        

        # Описание компонент модели
        user     = person "Пользователь"
        delivery = softwareSystem "Система доставки" {
            description "Сервер управления системой доставки"

            user_service = container "User service" {
                description "Сервис управления пользователями"
            }

            delivery_service = container "Delivery service" {
                description "Сервис управления доставками"
            }

            group "Слой данных" {
                user_database = container "User Database" {
                    description "База данных с пользователями и их доставками"
                    technology "PostgreSQL 15"
                    tags "database"
                }

                user_cache = container "User Cache" {
                    description "Кеш пользовательских данных для ускорения аутентификации"
                    technology "Redis"
                    tags "database"
                }

                delivery_database = container "Delivery Database" {
                    description "База данных для хранения информации о посылках"
                    technology "MongoDB 5"
                    tags "database"
                }
            }

            user_service -> user_database "Получение/обновление данных о пользователях" "TCP:5432"
            user_service -> user_cache "Получение/обновление данных о пользователях" "TCP:6379"

            delivery_service -> delivery_database "Получение/обновление данных о доставке" "TCP:27018"

            user -> user_service "Создание нового пользователя/получение информации о нем" "REST HTTP:8080"
            user -> delivery_service "Создание новой посылки/получение информации о ней" "REST HTTP:8080"
        }

        user -> delivery "Запросы к системе доставки" "REST HTTP:8080"

        deploymentEnvironment "Production" {
            deploymentNode "User Server" {
                containerInstance delivery.user_service
            }

            deploymentNode "Delivery Server" {
                containerInstance delivery.delivery_service
                properties {
                    "cpu" "4"
                    "ram" "256Gb"
                    "hdd" "4Tb"
                }
            }

            deploymentNode "databases" {
     
                deploymentNode "Database Server 1" {
                    containerInstance delivery.user_database
                }

                deploymentNode "Database Server 2" {
                    containerInstance delivery.delivery_database
                    instances 3
                }

                deploymentNode "Cache Server" {
                    containerInstance delivery.user_cache
                }
            }
            
        }
    }

    views {
        themes default

        properties { 
            structurizr.tooltips true
        }


        !script groovy {
            workspace.views.createDefaultViews()
            workspace.views.views.findAll { it instanceof com.structurizr.view.ModelView }.each { it.enableAutomaticLayout() }
        }

        dynamic delivery "UC01" "Создание нового пользователя" {
            autoLayout
            user -> delivery.user_service "Создать нового пользователя (POST /user)"
            delivery.user_service -> delivery.user_database "Сохранить данные о пользователе" 
            delivery.user_service -> delivery.user_cache "Сохранить данные о пользователе в кэш" 
        }

        dynamic delivery "UC02" "Поиск пользователя" {
            autoLayout
            user -> delivery.user_service "Найти пользователя (GET /user)"
            delivery.user_service -> delivery.user_cache "Найти данные пользователя"
            delivery.user_service -> delivery.user_database "Найти данные пользователя (если не нашли в кэше)"
        }

        dynamic delivery "UC03" "Создание посылки" {
            autoLayout
            user -> delivery.delivery_service "Создать новую посылку (POST /package)"
            delivery.delivery_service -> delivery.delivery_database "Сохранить данные о доставке" 
        }

        dynamic delivery "UC04" "Получение посылок пользователя" {
            autoLayout
            user -> delivery.delivery_service "Получить посылку (GET /package)"
            delivery.delivery_service -> delivery.delivery_database "Найти данные о доставке" 
        }

        dynamic delivery "UC05" "Создание доставки" {
            autoLayout
            user -> delivery.delivery_service "Создать новую доставку (POST /delivery)"
            delivery.delivery_service -> delivery.delivery_database "Сохранить данные о доставке" 
        }

        dynamic delivery "UC06" "Получение информации о доставке" {
            autoLayout
            user -> delivery.delivery_service "Получить информацию о доставке (GET /delivery)"
            delivery.delivery_service -> delivery.delivery_database "Найти данные о доставке" 
        }


        styles {
            element "database" {
                shape cylinder
            }
        }
    }
}