Distribution = Distribution or {} -- Distribution exists server-side but not client-side

function Distribution.FromTickUnit(value)
    return value * Constants.DistributionTickRate / 1000
end

function Distribution.FromStorageUnit(value)
    return value / 1000
end

function Distribution.ToStorageUnit(value)
    return value * 1000
end

function Distribution.ToTickUnit(value)
    return value * 1000 / Constants.DistributionTickRate
end
