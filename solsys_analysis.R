library(ggplot2)
library(reshape2)

display <- function(v){
    filename <- paste0("solsys_", v, ".csv")
    df <- read.csv(filename)
    df$happiness = 1000 * df$happiness
    df$culture = 1000 * df$culture
    df$storage = 1000 * df$storage
    df$ships = 100 * df$ships

    x <- melt(df, id.vars = c('entity', 'step'), measure.vars = c('population', 'happiness', 'ships', 'resource', 'culture', 'storage'))

    ggplot(x, aes(x = step, y = value, group = variable, color = variable)) + geom_path(aes(group = interaction(entity, variable)))
}
