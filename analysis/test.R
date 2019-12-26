df <- read.csv("test.csv")
ggplot(df, aes(x = t, y = x)) + geom_path() + geom_point()
ggplot(df, aes(x = t, y = y)) + geom_path() + geom_point()
