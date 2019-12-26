library(ggplot2)
library(reshape2)
library(plyr)

load.gov <- function(g) {
    filename <- paste0("solsys_", g, ".csv")
    read.csv(filename)
}

display <- function(v, facs, eids = NULL){
    df <- load.gov(v)
    if (!is.null(eids)) df <- subset(df, entity %in% eids)
    x <- melt(df, id.vars = c('entity', 'step'), measure.vars = facs)
    x$entity = as.factor(x$entity)

    ggplot(x, aes(x = step, y = value, group = variable, color = variable, shape = variable)) + geom_path(aes(group = interaction(entity, variable)))
}

compare.governors <- function(fac, govs, rounds = 160) {
    get <- function(g) {
        df <- load.gov(g)
        data.frame(governor = g, entity = df$entity, step = df$step, value = df[,fac])
    }

    data <- ldply(govs, get)

    ggplot(data, aes(x = step, y = value, color = governor, group = interaction(governor, entity))) + xlim(c(0, 50 * rounds)) + geom_path()
}
