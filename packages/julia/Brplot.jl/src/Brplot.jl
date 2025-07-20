module Brplot

using Brplot_jll

plot(y::Real)                    = @ccall Brplot_jll.libbrplot.brp_1(y::Cdouble, 0::Cint)::Cvoid
plot(y::Real, group_id::Integer) = @ccall Brplot_jll.libbrplot.brp_1(y::Cdouble, group_id::Cint)::Cvoid

plot(x::Real, y::AbstractFloat)           = @ccall Brplot_jll.libbrplot.brp_2(x::Cdouble, y::Cdouble, 0::Cint)::Cvoid
plot(x::Real, y::Real, group_id::Integer) = @ccall Brplot_jll.libbrplot.brp_2(x::Cdouble, y::Cdouble, group_id::Cint)::Cvoid

plot(x::Real, y::Real, z::AbstractFloat)           = @ccall Brplot_jll.libbrplot.brp_3(x::Cdouble, y::Cdouble, z::Cdouble, 0::Cint)::Cvoid
plot(x::Real, y::Real, z::Real, group_id::Integer) = @ccall Brplot_jll.libbrplot.brp_3(x::Cdouble, y::Cdouble, z::Cdouble, group_id::Cint)::Cvoid

label(value::String, group_id::Integer) = @ccall Brplot_jll.libbrplot.brp_label(value::Cstring, group_id::Cint)::Cvoid
label(value::String)                    = @ccall Brplot_jll.libbrplot.brp_label(value::Cstring, 0::Cint)::Cvoid

empty(group_id::Integer) = @ccall Brplot_jll.libbrplot.brp_empty(group_id::Cint)::Cvoid
focus_all()              = @ccall Brplot_jll.libbrplot.brp_focus_all()::Cvoid
flush()                  = @ccall Brplot_jll.libbrplot.brp_flush()::Cvoid
wait()                   = @ccall Brplot_jll.libbrplot.brp_wait()::Cvoid

end # module Brplot
